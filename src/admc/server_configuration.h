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

#ifndef SERVER_CONFIGURATION_H
#define SERVER_CONFIGURATION_H

#include <QString>

// Provides access to some server configuration data
// NOTE: it is assumed that a language change requires a restart
// so localized data is loaded once and is then reused after that

QString get_attribute_display_name(const QString &attribute, const QString &objectClass);
QString get_class_display_name(const QString &objectClass);
QList<QString> get_extra_contents_columns();
QList<QString> get_containers_filter_classes();
QList<QString> get_possible_superiors(const QString &dn);
QString get_ad_class_name(const QString &ldap_class_name);
QList<QString> get_possible_attributes(const QString &dn);

#endif /* SERVER_CONFIGURATION_H */