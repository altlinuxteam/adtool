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

#include "details_widget.h"
#include "attributes_widget.h"
#include "ad_interface.h"
#include "members_widget.h"
#include "settings.h"
#include "entry_context_menu.h"
#include "containers_widget.h"
#include "contents_widget.h"

#include <QTreeView>
#include <QStandardItemModel>
#include <QAction>

DetailsWidget::DetailsWidget(EntryContextMenu *entry_context_menu, ContainersWidget *containers_widget, ContentsWidget *contents_widget, QWidget *parent)
: QTabWidget(parent)
{
    members_widget = new MembersWidget(entry_context_menu, this);
    attributes_widget = new AttributesWidget(this);

    // Add all tabs to take ownership of them
    addTab(attributes_widget, "");
    addTab(members_widget, "");

    connect(
        AD(), &AdInterface::logged_in,
        this, &DetailsWidget::on_logged_in);
    connect(
        AD(), &AdInterface::modified,
        this, &DetailsWidget::on_ad_modified);

    connect(
        entry_context_menu, &EntryContextMenu::details,
        this, &DetailsWidget::on_context_menu_details);
    connect(
        containers_widget, &ContainersWidget::clicked_dn,
        this, &DetailsWidget::on_containers_clicked_dn);
    connect(
        contents_widget, &ContentsWidget::clicked_dn,
        this, &DetailsWidget::on_contents_clicked_dn);

    change_target("");
};

void DetailsWidget::change_target(const QString &dn) {
    // Save current tab/index to restore later
    QWidget *old_tab = widget(currentIndex());

    target_dn = dn;

    attributes_widget->change_target(target_dn);
    members_widget->change_target(target_dn);

    // Setup tabs
    clear();

    addTab(attributes_widget, "All Attributes");

    bool is_group = AD()->attribute_value_exists(target_dn, "objectClass", "group");
    if (is_group) {
        addTab(members_widget, "Group members");
    }

    // Restore current index if it is still shown
    // Otherwise current index is set to first tab by default
    const int old_tab_index_in_new_tabs = indexOf(old_tab);
    if (old_tab_index_in_new_tabs != -1) {
        setCurrentIndex(old_tab_index_in_new_tabs);
    }
}

void DetailsWidget::on_logged_in() {
    // Clear data on new login
    change_target("");
}

void DetailsWidget::on_ad_modified() {
    change_target(target_dn);
}

void DetailsWidget::on_containers_clicked_dn(const QString &dn) {
    if (SETTINGS()->details_on_containers_click->isChecked()) {
        change_target(dn);
    }
}

void DetailsWidget::on_contents_clicked_dn(const QString &dn) {
    if (SETTINGS()->details_on_contents_click->isChecked()) {
        change_target(dn);
    }
}

void DetailsWidget::on_context_menu_details(const QString &dn) {
    change_target(dn);
}
