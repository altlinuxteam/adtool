/*
 * ADMC - AD Management Center
 *
 * Copyright (C) 2020 BaseALT Ltd.
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

#include "customize_columns_dialog.h"

#include "results_description.h"

#include <QTreeView>
#include <QHeaderView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <QDebug>

CustomizeColumnsDialog::CustomizeColumnsDialog(ResultsDescription *results_arg, QWidget *parent)
: QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    results = results_arg;

    QTreeView *view = results->get_view();

    QHeaderView *header = view->header();
    QAbstractItemModel *model = view->model();

    auto checkboxes_widget = new QWidget();

    auto checkboxes_layout = new QVBoxLayout();
    checkboxes_widget->setLayout(checkboxes_layout);

    for (int i = 0; i < header->count(); i++) {
        const QString column_name = model->headerData(i, Qt::Horizontal).toString();

        auto checkbox = new QCheckBox(column_name);
        const bool currently_hidden = header->isSectionHidden(i);
        checkbox->setChecked(!currently_hidden);

        checkbox_list.append(checkbox);
    }

    for (int i = 0; i < header->count(); i++) {
        auto checkbox = checkbox_list[i];
        checkboxes_layout->addWidget(checkbox);
    }

    auto scroll_area = new QScrollArea();
    scroll_area->setWidget(checkboxes_widget);

    auto button_box = new QDialogButtonBox();
    auto ok_button = button_box->addButton(QDialogButtonBox::Ok);
    auto cancel_button = button_box->addButton(QDialogButtonBox::Cancel);
    auto restore_defaults_button = button_box->addButton(QDialogButtonBox::RestoreDefaults);

    auto dialog_layout = new QVBoxLayout();
    setLayout(dialog_layout);

    dialog_layout->addWidget(scroll_area);
    dialog_layout->addWidget(button_box);

    connect(
        ok_button, &QPushButton::clicked,
        this, &CustomizeColumnsDialog::accept);
    connect(
        cancel_button, &QPushButton::clicked,
        this, &QDialog::reject);
    connect(
        restore_defaults_button, &QPushButton::clicked,
        this, &CustomizeColumnsDialog::restore_defaults);
}

void CustomizeColumnsDialog::accept() {
    QTreeView *view = results->get_view();
    QHeaderView *header = view->header();

    for (int i = 0; i < checkbox_list.size(); i++) {
        QCheckBox *checkbox = checkbox_list[i];
        const bool hidden = !checkbox->isChecked();
        qInfo() << hidden;
        header->setSectionHidden(i, hidden);
    }

    QDialog::accept();
}

void CustomizeColumnsDialog::restore_defaults() {
    const QList<int> defaults = results->get_default_columns();

    for (int i = 0; i < checkbox_list.size(); i++) {
        QCheckBox *checkbox = checkbox_list[i];
        const bool hidden = !defaults.contains(i);
        checkbox->setChecked(!hidden);
    }
}
