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

#include "ad_interface.h"
#include "ad_connection.h"
#include "admc.h"

#include <QSet>
#include <algorithm>

AdInterface::AdInterface(QObject *parent)
: QObject(parent)
{
    connection = new adldap::AdConnection();

    connect(this, &AdInterface::modified,
        [this]() {
            attributes_cache.clear();
        });
}

AdInterface::~AdInterface() {
    delete connection;
}

// "CN=foo,CN=bar,DC=domain,DC=com"
// =>
// "foo"
QString extract_name_from_dn(const QString &dn) {
    int equals_i = dn.indexOf('=') + 1;
    int comma_i = dn.indexOf(',');
    int segment_length = comma_i - equals_i;

    QString name = dn.mid(equals_i, segment_length);

    return name;
}

// "CN=foo,CN=bar,DC=domain,DC=com"
// =>
// "CN=bar,DC=domain,DC=com"
QString extract_parent_dn_from_dn(const QString &dn) {
    int comma_i = dn.indexOf(',');

    QString parent_dn = dn.mid(comma_i + 1);

    return parent_dn;
}

void AdInterface::ad_interface_login(const QString &base, const QString &head) {
    connection->connect(base.toStdString(), head.toStdString());

    if (connection->is_connected()) {
        emit ad_interface_login_complete(base, head);
    } else {
        emit ad_interface_login_failed(base, head);
    }
}

QString AdInterface::get_error_str() {
    return QString(connection->get_errstr());
}

QString AdInterface::get_search_base() {
    return QString::fromStdString(connection->get_search_base());
}

QString AdInterface::get_uri() {
    return QString::fromStdString(connection->get_uri());
}

QList<QString> AdInterface::load_children(const QString &dn) {
    const QByteArray dn_array = dn.toLatin1();
    const char *dn_cstr = dn_array.constData();

    char **children_raw = connection->list(dn_cstr);

    if (children_raw != NULL) {
        auto children = QList<QString>();

        for (int i = 0; children_raw[i] != NULL; i++) {
            auto child = QString(children_raw[i]);
            children.push_back(child);
        }

        for (int i = 0; children_raw[i] != NULL; i++) {
            free(children_raw[i]);
        }
        free(children_raw);

        return children;
    } else {
        if (connection->get_errcode() != AD_SUCCESS) {
            emit load_children_failed(dn, get_error_str());
        }

        return QList<QString>();
    }
}

QList<QString> AdInterface::search(const QString &filter) {
    const QByteArray filter_array = filter.toLatin1();
    const char *filter_cstr = filter_array.constData();

    char **results_raw = connection->search(filter_cstr);
    int search_result = connection->get_errcode();

    if (search_result == AD_SUCCESS) {
        auto results = QList<QString>();

        for (int i = 0; results_raw[i] != NULL; i++) {
            auto result = QString(results_raw[i]);
            results.push_back(result);
        }

        for (int i = 0; results_raw[i] != NULL; i++) {
            free(results_raw[i]);
        }
        free(results_raw);

        return results;
    } else {
        emit search_failed(filter, get_error_str());

        return QList<QString>();
    }
}

Attributes AdInterface::load_attributes(const QString &dn) {
    const QByteArray dn_array = dn.toLatin1();
    const char *dn_cstr = dn_array.constData();

    char** attributes_raw = connection->get_attribute(dn_cstr, "*");

    // TODO: get_attribute is busted, doesn't return success correctly
    // so have to ignore result for now

    Attributes attributes;
    if (attributes_raw != NULL) {
        // Load attributes map
        // attributes_raw is in the form of:
        // char** array of {key, value, value, key, value ...}
        // transform it into:
        // map of {key => {value, value ...}, key => {value, value ...} ...}
        for (int i = 0; attributes_raw[i + 2] != NULL; i += 2) {
            auto attribute = QString(attributes_raw[i]);
            auto value = QString(attributes_raw[i + 1]);

            // Make values list if doesn't exist yet
            if (!attributes.contains(attribute)) {
                attributes[attribute] = QList<QString>();
            }

            attributes[attribute].push_back(value);
        }

        // Free attributes_raw
        for (int i = 0; attributes_raw[i] != NULL; i++) {
            free(attributes_raw[i]);
        }
        free(attributes_raw);
    }

    return attributes;
    
    // emit load_attributes_failed(dn, get_error_str());
}

Attributes AdInterface::get_attributes(const QString &dn) {
    Attributes attributes;

    if (dn == "") {
        attributes = Attributes();
    } else if (attributes_cache.contains(dn)) {
        attributes = attributes_cache[dn];
    } else {
        attributes = load_attributes(dn);
        attributes_cache[dn] = attributes;
    }

    return attributes;
}

QList<QString> AdInterface::get_attribute_multi(const QString &dn, const QString &attribute) {
    QMap<QString, QList<QString>> attributes = get_attributes(dn);

    if (attributes.contains(attribute)) {
        return attributes[attribute];
    } else {
        return QList<QString>();
    }
}

QString AdInterface::get_attribute(const QString &dn, const QString &attribute) {
    QList<QString> values = get_attribute_multi(dn, attribute);

    if (values.size() > 0) {
        // Return first value only
        return values[0];
    } else {
        return "";
    }
}

bool AdInterface::attribute_value_exists(const QString &dn, const QString &attribute, const QString &value) {
    QList<QString> values = get_attribute_multi(dn, attribute);

    if (values.contains(value)) {
        return true;
    } else {
        return false;
    }
}

bool AdInterface::set_attribute(const QString &dn, const QString &attribute, const QString &value) {
    int result = AD_INVALID_DN;

    const QString old_value = get_attribute(dn, attribute);
    
    const QByteArray dn_array = dn.toLatin1();
    const char *dn_cstr = dn_array.constData();

    const QByteArray attribute_array = attribute.toLatin1();
    const char *attribute_cstr = attribute_array.constData();

    const QByteArray value_array = value.toLatin1();
    const char *value_cstr = value_array.constData();

    result = connection->mod_replace(dn_cstr, attribute_cstr, value_cstr);

    if (result == AD_SUCCESS) {
        // Reload attributes to get new value
        load_attributes(dn);
        
        emit set_attribute_complete(dn, attribute, old_value, value);
        emit modified();

        return true;
    } else {
        emit set_attribute_failed(dn, attribute, old_value, value, get_error_str());

        return false;
    }
}

// TODO: can probably make a create_anything() function with enum parameter
bool AdInterface::create_entry(const QString &name, const QString &dn, NewEntryType type) {
    int result = AD_INVALID_DN;
    
    const QByteArray name_array = name.toLatin1();
    const char *name_cstr = name_array.constData();

    const QByteArray dn_array = dn.toLatin1();
    const char *dn_cstr = dn_array.constData();

    switch (type) {
        case User: {
            result = connection->create_user(name_cstr, dn_cstr);
            break;
        }
        case Computer: {
            result = connection->create_computer(name_cstr, dn_cstr);
            break;
        }
        case OU: {
            result = connection->ou_create(name_cstr, dn_cstr);
            break;
        }
        case Group: {
            result = connection->group_create(name_cstr, dn_cstr);
            break;
        }
        case COUNT: break;
    }

    if (result == AD_SUCCESS) {
        emit create_entry_complete(dn, type);
        emit modified();

        return true;
    } else {
        emit create_entry_failed(dn, type, get_error_str());

        return false;
    }
}

void AdInterface::delete_entry(const QString &dn) {
    int result = AD_INVALID_DN;

    const QByteArray dn_array = dn.toLatin1();
    const char *dn_cstr = dn_array.constData();

    result = connection->object_delete(dn_cstr);

    if (result == AD_SUCCESS) {
        emit delete_entry_complete(dn);
        emit modified();
    } else {
        emit delete_entry_failed(dn, get_error_str());
    }
}

void AdInterface::move(const QString &dn, const QString &new_container) {
    int result = AD_INVALID_DN;

    QList<QString> dn_split = dn.split(',');
    QString new_dn = dn_split[0] + "," + new_container;

    const QByteArray dn_array = dn.toLatin1();
    const char *dn_cstr = dn_array.constData();

    const QByteArray new_container_array = new_container.toLatin1();
    const char *new_container_cstr = new_container_array.constData();

    const bool entry_is_user = is_user(dn);
    if (entry_is_user) {
        result = connection->move_user(dn_cstr, new_container_cstr);
    } else {
        result = connection->move(dn_cstr, new_container_cstr);
    }

    // TODO: drag and drop handles checking move compatibility but need
    // to do this here as well for CLI?
    
    if (result == AD_SUCCESS) {
        emit move_complete(dn, new_container, new_dn);
        emit modified();
    } else {
        emit move_failed(dn, new_container, new_dn, get_error_str());
    }
}

void AdInterface::add_user_to_group(const QString &group_dn, const QString &user_dn) {
    int result = AD_INVALID_DN;

    const QByteArray group_dn_array = group_dn.toLatin1();
    const char *group_dn_cstr = group_dn_array.constData();

    const QByteArray user_dn_array = user_dn.toLatin1();
    const char *user_dn_cstr = user_dn_array.constData();

    result = connection->group_add_user(group_dn_cstr, user_dn_cstr);

    if (result == AD_SUCCESS) {
        emit add_user_to_group_complete(group_dn, user_dn);
        emit modified();
    } else {
        emit add_user_to_group_failed(group_dn, user_dn, get_error_str());
    }
}

void AdInterface::group_remove_user(const QString &group_dn, const QString &user_dn) {
    int result = AD_INVALID_DN;

    const QByteArray group_dn_array = group_dn.toLatin1();
    const char *group_dn_cstr = group_dn_array.constData();

    const QByteArray user_dn_array = user_dn.toLatin1();
    const char *user_dn_cstr = user_dn_array.constData();

    result = connection->group_remove_user(group_dn_cstr, user_dn_cstr);

    if (result == AD_SUCCESS) {
        emit group_remove_user_complete(group_dn, user_dn);
        emit modified();
    } else {
        emit group_remove_user_failed(group_dn, user_dn, get_error_str());
    }
}

void AdInterface::rename(const QString &dn, const QString &new_name) {
    // Compose new_rdn and new_dn
    const QStringList exploded_dn = dn.split(',');
    const QString old_rdn = exploded_dn[0];
    const int prefix_i = old_rdn.indexOf('=') + 1;
    const QString prefix = old_rdn.left(prefix_i);
    const QString new_rdn = prefix + new_name;
    QStringList new_exploded_dn(exploded_dn);
    new_exploded_dn.replace(0, new_rdn);
    const QString new_dn = new_exploded_dn.join(',');

    const QByteArray dn_array = dn.toLatin1();
    const char *dn_cstr = dn_array.constData();
    const QByteArray new_name_array = new_name.toLatin1();
    const char *new_name_cstr = new_name_array.constData();
    const QByteArray new_rdn_array = new_rdn.toLatin1();
    const char *new_rdn_cstr = new_rdn_array.constData();

    int result = AD_INVALID_DN;
    if (is_user(dn)) {
        result = connection->rename_user(dn_cstr, new_name_cstr);
    } else if (is_group(dn)) {
        result = connection->rename_group(dn_cstr, new_name_cstr);
    } else {
        result = connection->rename(dn_cstr, new_rdn_cstr);
    }

    if (result == AD_SUCCESS) {
        emit rename_complete(dn, new_name, new_dn);
        emit modified();
    } else {
        emit rename_failed(dn, new_name, new_dn, get_error_str());
    }
}

bool AdInterface::is_user(const QString &dn) {
    return attribute_value_exists(dn, "objectClass", "user");
}

bool AdInterface::is_group(const QString &dn) {
    return attribute_value_exists(dn, "objectClass", "group");
}

bool AdInterface::is_container(const QString &dn) {
    return attribute_value_exists(dn, "objectClass", "container");
}

bool AdInterface::is_ou(const QString &dn) {
    return attribute_value_exists(dn, "objectClass", "organizationalUnit");
}

bool AdInterface::is_policy(const QString &dn) {
    return attribute_value_exists(dn, "objectClass", "groupPolicyContainer");
}

bool AdInterface::is_container_like(const QString &dn) {
    // TODO: check that this includes all fitting objectClasses
    const QList<QString> containerlike_objectClasses = {"organizationalUnit", "builtinDomain", "domain"};
    for (auto c : containerlike_objectClasses) {
        if (AD()->attribute_value_exists(dn, "objectClass", c)) {
            return true;
        }
    }

    return false;
}

enum DropType {
    DropType_Move,
    DropType_AddToGroup,
    DropType_None
};

// Determine what kind of drop type is dropping this entry onto target
// If drop type is none, then can't drop this entry on this target
DropType get_drop_type(const QString &dn, const QString &target_dn) {
    if (dn == target_dn) {
        return DropType_None;
    }

    const bool dropped_is_user = AD()->is_user(dn);
    const bool dropped_is_group = AD()->is_group(dn);
    const bool dropped_is_ou = AD()->is_ou(dn);

    const bool target_is_user = AD()->is_user(target_dn);
    const bool target_is_group = AD()->is_group(target_dn);
    const bool target_is_ou = AD()->is_ou(target_dn);
    const bool target_is_container = AD()->is_container(target_dn);

    if (dropped_is_user) {
        if (target_is_ou || target_is_container) {
            return DropType_Move;
        } else if (target_is_group) {
            return DropType_AddToGroup;
        }
    } else if (dropped_is_group || dropped_is_ou) {
        if (!target_is_user && !target_is_group) {
            return DropType_Move;
        }
    }

    return DropType_None;
}

bool AdInterface::can_drop_entry(const QString &dn, const QString &target_dn) {
    DropType drop_type = get_drop_type(dn, target_dn);

    if (drop_type == DropType_None) {
        return false;
    } else {
        return true;
    }
}

// General "drop" operation that can either move, link or change membership depending on which types of entries are involved
void AdInterface::drop_entry(const QString &dn, const QString &target_dn) {
    DropType drop_type = get_drop_type(dn, target_dn);

    switch (drop_type) {
        case DropType_Move: {
            AD()->move(dn, target_dn);
            break;
        }
        case DropType_AddToGroup: {
            AD()->add_user_to_group(target_dn, dn);
            break;
        }
        case DropType_None: {
            break;
        }
    }
}

void AdInterface::command(QStringList args) {
    QString command = args[0];

    QMap<QString, int> arg_count_map = {
        {"list", 1},
        {"get-attribute", 2},
        {"get-attribute-multi", 2},
    };

    const int arg_count = arg_count_map[command];
    if (args.size() - 1 != arg_count) {
        printf("Command \"%s\" needs %d arguments!\n", qPrintable(command), arg_count);

        return;
    }

    if (command == "list") {
        QString dn = args[1];

        QList<QString> children = load_children(dn);

        for (auto e : children) {
            printf("%s\n", qPrintable(e));
        }
    } else if (command == "get-attribute") {
        QString dn = args[1];
        QString attribute = args[2];

        QString value = get_attribute(dn, attribute);

        printf("%s\n", qPrintable(value));
    } else if (command == "get-attribute-multi") {
        QString dn = args[1];
        QString attribute = args[2];

        QList<QString> values = get_attribute_multi(dn, attribute);

        for (auto e : values) {
            printf("%s\n", qPrintable(e));
        }
    }
}

AdInterface *AD() {
    ADMC *app = qobject_cast<ADMC *>(qApp);
    AdInterface *ad = app->ad_interface();
    return ad;
}

QString filter_EQUALS(const QString &attribute, const QString &value) {
    auto filter = QString("(%1=%2)").arg(attribute, value);
    return filter;
}

QString filter_AND(const QString &a, const QString &b) {
    auto filter = QString("(&%1%2)").arg(a, b);
    return filter;
}
QString filter_OR(const QString &a, const QString &b) {
    auto filter = QString("(|%1%2)").arg(a, b);
    return filter;
}
QString filter_NOT(const QString &a) {
    auto filter = QString("(!%1)").arg(a);
    return filter;
}
