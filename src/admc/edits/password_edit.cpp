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

#include "edits/password_edit.h"

#include "adldap.h"
#include "globals.h"
#include "utils.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QTextCodec>

PasswordEdit::PasswordEdit(QList<AttributeEdit *> *edits_out, QObject *parent)
: AttributeEdit(edits_out, parent) {
    edit = new QLineEdit();
    confirm_edit = new QLineEdit();

    edit->setObjectName("password_main_edit");
    confirm_edit->setObjectName("password_confirm_edit");

    edit->setEchoMode(QLineEdit::Password);
    confirm_edit->setEchoMode(QLineEdit::Password);

    limit_edit(edit, ATTRIBUTE_PASSWORD);
    limit_edit(confirm_edit, ATTRIBUTE_PASSWORD);

    QObject::connect(
        edit, &QLineEdit::textChanged,
        [this]() {
            emit edited();
        });
}

void PasswordEdit::load_internal(AdInterface &ad, const AdObject &object) {
    edit->clear();
    confirm_edit->clear();
}

void PasswordEdit::set_read_only(const bool read_only) {
    edit->setReadOnly(read_only);
    confirm_edit->setReadOnly(read_only);
}

void PasswordEdit::add_to_layout(QFormLayout *layout) {
    layout->addRow(tr("Password:"), edit);
    layout->addRow(tr("Confirm password:"), confirm_edit);
}

bool PasswordEdit::verify(AdInterface &ad, const QString &) const {
    const QString pass = edit->text();
    const QString confirm_pass = confirm_edit->text();
    if (pass != confirm_pass) {
        const QString error_text = QString(tr("Passwords don't match!"));
        message_box_warning(edit, tr("Error"), error_text);

        return false;
    }

    const auto codec = QTextCodec::codecForName("UTF-16LE");
    const bool can_encode = codec->canEncode(pass);
    if (!can_encode) {
        const QString error_text = QString(tr("Password contains invalid characters"));
        message_box_warning(edit, tr("Error"), error_text);

        return false;
    }

    return true;
}

bool PasswordEdit::apply(AdInterface &ad, const QString &dn) const {
    const QString new_value = edit->text();

    const bool success = ad.user_set_pass(dn, new_value);

    return success;
}
