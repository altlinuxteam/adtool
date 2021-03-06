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

#include "edits/expiry_widget.h"

#include "adldap.h"
#include "globals.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDateEdit>
#include <QDateTime>
#include <QVBoxLayout>

// TODO: do end of day by local time or UTC????

const QTime END_OF_DAY(23, 59);

ExpiryWidget::ExpiryWidget()
: QFrame() {
    setFrameStyle(QFrame::Raised);
    setFrameShape(QFrame::Box);

    never_check = new QCheckBox(tr("Never"));
    never_check->setObjectName("never_check");

    end_of_check = new QCheckBox(tr("End of:"));
    end_of_check->setObjectName("end_of_check");

    never_check->setAutoExclusive(true);
    end_of_check->setAutoExclusive(true);

    edit = new QDateEdit();
    edit->setObjectName("date_edit");

    auto button_group = new QButtonGroup(this);
    button_group->addButton(never_check);
    button_group->addButton(end_of_check);

    auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->addWidget(never_check);
    layout->addWidget(end_of_check);
    layout->addWidget(edit);

    connect(
        never_check, &QCheckBox::stateChanged,
        this, &ExpiryWidget::on_never_check);
    connect(
        end_of_check, &QCheckBox::stateChanged,
        this, &ExpiryWidget::on_end_of_check);
    connect(
        edit, &QDateEdit::dateChanged,
        [this]() {
            emit edited();
        });
}

void ExpiryWidget::load(const AdObject &object) {
    const bool never = [object]() {
        const QString expiry_string = object.get_string(ATTRIBUTE_ACCOUNT_EXPIRES);
        return large_integer_datetime_is_never(expiry_string);
    }();

    if (never) {
        never_check->setChecked(true);

        end_of_check->setChecked(false);
        edit->setEnabled(false);
    } else {
        never_check->setChecked(false);

        end_of_check->setChecked(true);
        edit->setEnabled(true);
    }

    const QDate date = [=]() {
        if (never) {
            // Default to current date when expiry is never
            return QDate::currentDate();
        } else {
            const QDateTime datetime = object.get_datetime(ATTRIBUTE_ACCOUNT_EXPIRES, g_adconfig);
            return datetime.date();
        }
    }();

    edit->setDate(date);
}

void ExpiryWidget::set_read_only(const bool read_only) {
    never_check->setDisabled(read_only);
    end_of_check->setDisabled(read_only);
    edit->setReadOnly(read_only);
}

bool ExpiryWidget::apply(AdInterface &ad, const QString &dn) const {
    const bool never = never_check->isChecked();

    if (never) {
        return ad.attribute_replace_string(dn, ATTRIBUTE_ACCOUNT_EXPIRES, AD_LARGE_INTEGER_DATETIME_NEVER_2);
    } else {
        const QDate date = edit->date();
        const QDateTime datetime(date, END_OF_DAY);

        return ad.attribute_replace_datetime(dn, ATTRIBUTE_ACCOUNT_EXPIRES, datetime);
    }
}

void ExpiryWidget::on_never_check() {
    if (never_check->isChecked()) {
        edit->setEnabled(false);

        emit edited();
    }
}

void ExpiryWidget::on_end_of_check() {
    if (end_of_check->isChecked()) {
        edit->setEnabled(true);

        emit edited();
    }
}
