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

#include "editors/datetime_editor.h"

#include "adldap.h"
#include "globals.h"

#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

DateTimeEditor::DateTimeEditor(const QString attribute_arg, QWidget *parent)
: AttributeEditor(parent) {
    setWindowTitle(tr("Edit Datetime"));

    attribute = attribute_arg;

    QLabel *attribute_label = make_attribute_label(attribute);

    edit = new QDateTimeEdit();

    QDialogButtonBox *button_box = make_button_box(attribute);
    ;

    const auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->addWidget(attribute_label);
    layout->addWidget(edit);
    layout->addWidget(button_box);

    const bool system_only = g_adconfig->get_attribute_is_system_only(attribute);
    if (system_only) {
        edit->setReadOnly(true);
    }
}

void DateTimeEditor::load(const QList<QByteArray> &values) {
    const QByteArray value = values.value(0, QByteArray());
    const QString value_string = QString(value);
    const QDateTime value_datetime = datetime_string_to_qdatetime(attribute, value_string, g_adconfig);
    edit->setDateTime(value_datetime);
}

QList<QByteArray> DateTimeEditor::get_new_values() const {
    return QList<QByteArray>();
}
