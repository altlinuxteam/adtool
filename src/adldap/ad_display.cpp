/*
 * ADMC - AD Management Center
 *
 * Copyright (C) 2020-2021 BaseALT Ltd.
 * Copyright (C) 2020-2021 Dmitry Degtyarev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ad_display.h"

#include "ad_config.h"
#include "ad_defines.h"
#include "ad_utils.h"
#include "samba/dom_sid.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QList>
#include <QString>
#include <algorithm>

const qint64 SECONDS_TO_MILLIS = 1000LL;
const qint64 MINUTES_TO_SECONDS = 60LL;
const qint64 HOURS_TO_SECONDS = MINUTES_TO_SECONDS * 60LL;
const qint64 DAYS_TO_SECONDS = HOURS_TO_SECONDS * 24LL;

QString large_integer_datetime_display_value(const QString &attribute, const QByteArray &bytes, const AdConfig *adconfig);
QString datetime_display_value(const QString &attribute, const QByteArray &bytes, const AdConfig *adconfig);
QString timespan_display_value(const QByteArray &bytes);
QString octet_display_value(const QByteArray &bytes);
QString guid_to_display_value(const QByteArray &bytes);

QString attribute_display_value(const QString &attribute, const QByteArray &value, const AdConfig *adconfig) {
    if (adconfig == nullptr) {
        return value;
    }

    const AttributeType type = adconfig->get_attribute_type(attribute);

    switch (type) {
        case AttributeType_LargeInteger: {
            const LargeIntegerSubtype subtype = adconfig->get_attribute_large_integer_subtype(attribute);

            switch (subtype) {
                case LargeIntegerSubtype_Datetime: return large_integer_datetime_display_value(attribute, value, adconfig);
                case LargeIntegerSubtype_Timespan: return timespan_display_value(value);
                case LargeIntegerSubtype_Integer: return QString(value);
            }
        }
        case AttributeType_UTCTime: return datetime_display_value(attribute, value, adconfig);
        case AttributeType_GeneralizedTime: return datetime_display_value(attribute, value, adconfig);
        case AttributeType_Sid: return object_sid_display_value(value);
        case AttributeType_Octet: {
            if (attribute == ATTRIBUTE_OBJECT_GUID) {
                return guid_to_display_value(value);
            } else {
                return octet_display_value(value);
            }
        }
        case AttributeType_NTSecDesc: {
            return QCoreApplication::translate("attribute_display", "<BINARY VALUE>");
        }
        default: {
            return QString(value);
        }
    }
}

QString attribute_display_values(const QString &attribute, const QList<QByteArray> &values, const AdConfig *adconfig) {
    if (values.isEmpty()) {
        return QCoreApplication::translate("attribute_display", "<unset>");
    } else {
        QString out;

        // Convert values list to
        // "display_value1;display_value2;display_value3..."
        for (int i = 0; i < values.size(); i++) {
            if (i > 0) {
                out += ";";
            }

            const QByteArray value = values[i];
            const QString display_value = attribute_display_value(attribute, value, adconfig);

            out += display_value;
        }

        return out;
    }
}

QString object_sid_display_value(const QByteArray &sid_bytes) {
    dom_sid *sid = (dom_sid *) sid_bytes.data();

    TALLOC_CTX *tmp_ctx = talloc_new(NULL);

    const char *sid_cstr = dom_sid_string(tmp_ctx, sid);
    const QString out = QString(sid_cstr);

    talloc_free(tmp_ctx);

    return out;
}

QString large_integer_datetime_display_value(const QString &attribute, const QByteArray &value, const AdConfig *adconfig) {
    const QString value_string = QString(value);

    if (large_integer_datetime_is_never(value_string)) {
        return QCoreApplication::translate("attribute_display", "(never)");
    } else {
        QDateTime datetime = datetime_string_to_qdatetime(attribute, value_string, adconfig);
        const QString display = datetime.toLocalTime().toString(DATETIME_DISPLAY_FORMAT);

        return display;
    }
}

QString datetime_display_value(const QString &attribute, const QByteArray &bytes, const AdConfig *adconfig) {
    const QString value_string = QString(bytes);
    const QDateTime datetime = datetime_string_to_qdatetime(attribute, value_string, adconfig);
    const QDateTime datetime_local = datetime.toLocalTime();
    const QString display = datetime_local.toString(DATETIME_DISPLAY_FORMAT) + datetime.toLocalTime().timeZoneAbbreviation();

    return display;
}

QString timespan_display_value(const QByteArray &bytes) {
    // Timespan = integer value of hundred nanosecond quantities
    // (also negated)
    // Convert to dd:hh:mm:ss
    const QString value_string = QString(bytes);
    const qint64 hundred_nanos_negative = value_string.toLongLong();

    if (hundred_nanos_negative == LLONG_MIN) {
        return "(never)";
    }

    qint64 seconds_total = [hundred_nanos_negative]() {
        const qint64 hundred_nanos = -hundred_nanos_negative;
        const qint64 millis = hundred_nanos / MILLIS_TO_100_NANOS;
        const qint64 seconds = millis / SECONDS_TO_MILLIS;

        return seconds;
    }();

    const qint64 days = seconds_total / DAYS_TO_SECONDS;
    seconds_total = days % DAYS_TO_SECONDS;

    const qint64 hours = seconds_total / HOURS_TO_SECONDS;
    seconds_total = hours % HOURS_TO_SECONDS;

    const qint64 minutes = seconds_total / MINUTES_TO_SECONDS;
    seconds_total = minutes % MINUTES_TO_SECONDS;

    const qint64 seconds = seconds_total;

    // Convert arbitrary time unit value to string with format
    // "00-99" with leading zero if needed
    const auto time_unit_to_string = [](qint64 time) -> QString {
        if (time > 99) {
            time = 99;
        }

        const QString time_string = QString::number(time);

        if (time == 0) {
            return "00";
        } else if (time < 10) {
            return "0" + time_string;
        } else {
            return time_string;
        }
    };

    const QString days_string = time_unit_to_string(days);
    const QString hours_string = time_unit_to_string(hours);
    const QString minutes_string = time_unit_to_string(minutes);
    const QString seconds_string = time_unit_to_string(seconds);

    const QString display = QString("%1:%2:%3:%4").arg(days_string, hours_string, minutes_string, seconds_string);

    return display;
}

QString guid_to_display_value(const QByteArray &bytes) {
    // NOTE: have to do some weird pre-processing to match
    // how Windows displays GUID's. The GUID is broken down
    // into 5 segments, each separated by '-':
    // "0000-11-22-33-444444". Byte order for first 3
    // segments is also reversed (why?), so reverse it again
    // for display.
    const int segments_count = 5;
    QByteArray segments[segments_count];
    segments[0] = bytes.mid(0, 4);
    segments[1] = bytes.mid(4, 2);
    segments[2] = bytes.mid(6, 2);
    segments[3] = bytes.mid(8, 2);
    segments[4] = bytes.mid(10, 6);
    std::reverse(segments[0].begin(), segments[0].end());
    std::reverse(segments[1].begin(), segments[1].end());
    std::reverse(segments[2].begin(), segments[2].end());

    const QString guid_display_string = [&]() {
        QString out;

        for (int i = 0; i < segments_count; i++) {
            const QByteArray segment = segments[i];

            if (i > 0) {
                out += '-';
            }

            out += segment.toHex();
        }

        return out;
    }();

    return guid_display_string;
}

QString octet_display_value(const QByteArray &bytes) {
    const QByteArray bytes_hex = bytes.toHex();

    QByteArray out = bytes_hex;

    // Add " 0x" before each byte (every 2 indexes)
    for (int i = out.size() - 2; i >= 0; i -= 2) {
        out.insert(i, "0x");

        if (i != 0) {
            out.insert(i, " ");
        }
    }

    return QString(out);
}
