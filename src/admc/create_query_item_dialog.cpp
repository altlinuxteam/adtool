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

#include "create_query_item_dialog.h"

#include "console_types/console_query.h"
#include "edit_query_item_widget.h"
#include "utils.h"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QModelIndex>

CreateQueryItemDialog::CreateQueryItemDialog(ConsoleWidget *console_arg)
: QDialog(console_arg) {
    setAttribute(Qt::WA_DeleteOnClose);

    console = console_arg;

    const auto title = QString(tr("Create Query"));
    setWindowTitle(title);

    edit_query_widget = new EditQueryItemWidget();

    auto button_box = new QDialogButtonBox();
    button_box->addButton(QDialogButtonBox::Ok);
    button_box->addButton(QDialogButtonBox::Cancel);

    const auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->addWidget(edit_query_widget);
    layout->addWidget(button_box);

    connect(
        button_box, &QDialogButtonBox::accepted,
        this, &QDialog::accept);
    connect(
        button_box, &QDialogButtonBox::rejected,
        this, &QDialog::reject);
}

void CreateQueryItemDialog::accept() {
    QString name;
    QString description;
    QString filter;
    QString base;
    QByteArray filter_state;
    bool scope_is_children;
    edit_query_widget->save(name, description, filter, base, scope_is_children, filter_state);

    const QModelIndex parent_index = console->get_selected_item();

    if (!console_query_or_folder_name_is_good(console, name, parent_index, this, QModelIndex())) {
        return;
    }

    console_query_item_create(console, name, description, filter, filter_state, base, scope_is_children, parent_index);

    console_query_tree_save(console);

    QDialog::accept();
}
