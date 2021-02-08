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

#include "test_admc.h"

#include "ad_interface.h"
#include "ad_defines.h"
#include "ad_object.h"
#include "ad_utils.h"
#include "filter.h"
#include "create_dialog.h"
#include "utils.h"
#include "status.h"
#include "object_menu.h"
#include "object_model.h"
#include "select_container_dialog.h"

#include <QTest>
#include <QDebug>
#include <QModelIndex>
#include <QTreeView>

#define TEST_USER "test-user"
#define TEST_USER_LOGON "test-user-logon"
#define TEST_PASSWORD "pass123!"
#define TEST_OU "test-ou"

void TestADMC::initTestCase() {
    const bool connected = AD()->connect();
    QVERIFY2(connected, "Failed to connect to AD server");

    // NOTE: temporary band-aid until messages are routed correctly throgh AdInterface instance. This makes status error messages be printed to console. I think it's useful to understand why a test failed. When messages are collected in an AdInterface instances, can just print them here ourselves and avoid touching Status.
    STATUS()->print_errors = true;

    // Cleanup before all tests in-case this test suite was
    // previously interrupted and a cleanup wasn't performed
    cleanup();
}

void TestADMC::cleanupTestCase() {

}

void TestADMC::init() {
    parent_widget = new QWidget();
    
    const QString dn = test_arena_dn();
    const bool create_success = AD()->object_add(dn, CLASS_OU);

    QVERIFY2(create_success, "Failed to create test-arena");
}

// NOTE: can't just delete test arena while it has children
// because LDAP forbids deleting non-leaf objects. So need
// to delete leaves first.
void TestADMC::delete_test_arena_recursive(const QString &parent) {
    const QHash<QString, AdObject> search_results = AD()->search(QString(), QList<QString>(), SearchScope_Children, parent);

    const bool has_children = (search_results.size() > 0);
    if (has_children) {
        for (const QString child : search_results.keys()) {
            delete_test_arena_recursive(child);
        }
    }

    const bool delete_success = AD()->object_delete(parent);
    QVERIFY2(delete_success, "Failed to delete test-arena or it's contents");
}

void TestADMC::cleanup() {
    if (parent_widget != nullptr) {
        delete parent_widget;
        parent_widget = nullptr;
    }

    // Delete test arena, if it exists
    const QString dn = test_arena_dn();

    const QHash<QString, AdObject> search_results = AD()->search(QString(), QList<QString>(), SearchScope_Object, dn);
    const bool test_arena_exists = (search_results.size() == 1);
    if (test_arena_exists) {
        delete_test_arena_recursive(dn);
    }
}

void TestADMC::object_add() {
    const QString name = TEST_USER;
    const QString dn = test_object_dn(name, CLASS_USER);

    const bool create_success = AD()->object_add(dn, CLASS_USER);

    QVERIFY2(create_success, "Failed to create object");
    QVERIFY2(object_exists(dn), "Created object doesn't exist");
}

void TestADMC::object_delete() {
    const QString name = TEST_USER;
    const QString dn = test_object_dn(name, CLASS_USER);

    const bool create_success = AD()->object_add(dn, CLASS_USER);
    QVERIFY2(create_success, "Failed to create object for deletion");

    const bool delete_success = AD()->object_delete(dn);
    QVERIFY2(delete_success, "Failed to delete object");
    QVERIFY2(!object_exists(dn), "Deleted object exists");
}

void TestADMC::create_dialog_user() {
    const QString name = TEST_USER;
    const QString logon_name = TEST_USER_LOGON;
    const QString password = TEST_PASSWORD;
    const QString parent = test_arena_dn();
    const QString dn = test_object_dn(name, CLASS_USER);

    // Create user
    create(parent, CLASS_USER, parent_widget);
    auto create_dialog = parent_widget->findChild<CreateDialog *>();
    QVERIFY2((create_dialog != nullptr), "Failed to find create dialog");
    QApplication::setActiveWindow(create_dialog);

    // Enter name
    QTest::keyClicks(QApplication::focusWidget(), name);

    // Enter logon name
    tab(4);
    QTest::keyClicks(QApplication::focusWidget(), logon_name);

    // Enter password
    tab(2);
    QTest::keyClicks(QApplication::focusWidget(), password);
    // Confirm password
    tab();
    QTest::keyClicks(QApplication::focusWidget(), password);

    create_dialog->accept();

    QVERIFY2(object_exists(dn), "Created user doesn't exist");

    QVERIFY(true);
}

void TestADMC::create_dialog_ou() {
    const QString name = TEST_OU;
    const QString parent = test_arena_dn();
    const QString dn = test_object_dn(name, CLASS_OU);

    // Create ou
    create(parent, CLASS_OU, parent_widget);
    auto create_dialog = parent_widget->findChild<CreateDialog *>();
    QApplication::setActiveWindow(create_dialog);

    // Enter name
    QTest::keyClicks(QApplication::focusWidget(), name);

    create_dialog->accept();

    QVERIFY2(object_exists(dn), "Created OU doesn't exist");

    QVERIFY(true);
}

void TestADMC::object_menu_move() {
    const QString parent = test_arena_dn();
    
    const QString user_name = TEST_USER;
    const QString user_dn = test_object_dn(user_name, CLASS_USER);

    const QString move_target_name = "move-target-ou";
    const QString move_target_dn = test_object_dn(move_target_name, CLASS_OU);

    const QString user_dn_after_move = dn_from_name_and_parent(user_name, move_target_dn, CLASS_USER);

    // Create test user
    const bool create_user_success = AD()->object_add(user_dn, CLASS_USER);
    QVERIFY2(create_user_success, "Failed to create user");
    QVERIFY2(object_exists(user_dn), "Created user doesn't exist");

    // Create move target
    const bool create_move_target_success = AD()->object_add(move_target_dn, CLASS_OU);
    QVERIFY2(create_move_target_success, "Failed to create move target");
    QVERIFY2(object_exists(move_target_dn), "Created move target doesn't exist");

    // Open move dialog
    move({user_dn}, parent_widget);
    auto move_dialog = parent_widget->findChild<SelectContainerDialog *>();
    QVERIFY2((move_dialog != nullptr), "Failed to find move dialog");
    QApplication::setActiveWindow(move_dialog);

    QTreeView *move_dialog_view = qobject_cast<QTreeView *>(QApplication::focusWidget());
    QVERIFY2((move_dialog_view != nullptr), "Failed to cast move_dialog_view");

    // Helper f-ns:
    // TODO: probably will be reused to extract them

    // Press right to expand view item
    auto expand_current =
    []() {
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Right);
    };

    // Go down the list of objects by pressing Down arrow
    // until current item's dn equals to target dn
    auto navigate_until_object =
    [=](const QString &target_dn){
        QModelIndex current_index = move_dialog_view->selectionModel()->currentIndex();
        QModelIndex prev_index;
        while (current_index != prev_index) {
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Down);

            current_index = move_dialog_view->selectionModel()->currentIndex();

            const QString current_dn = current_index.data(Role_DN).toString();

            if (current_dn == target_dn) {
                break;
            }
        }
    };

    // Select move target in the view
    // First current item is the head
    expand_current();
    navigate_until_object(test_arena_dn());
    expand_current();
    navigate_until_object(move_target_dn);

    move_dialog->accept();

    QVERIFY2(object_exists(user_dn_after_move), "Moved object doesn't exist");

    QVERIFY(true);
}

QString TestADMC::test_arena_dn() {
    const QString head_dn = AD()->domain_head();
    const QString dn = QString("OU=test-arena,%1").arg(head_dn);

    return dn;
}

// TODO: for OU's need to change from cn to ou, SO make a f-n in ad_utils.cpp that does what's currently in CreateDialog::accept(). Takes object name, parent dn and object class, returns dn.
QString TestADMC::test_object_dn(const QString &name, const QString &object_class) {
    const QString parent = test_arena_dn();
    const QString dn = dn_from_name_and_parent(name, parent, object_class);

    return dn;
}

bool TestADMC::object_exists(const QString &dn) {
    const QHash<QString, AdObject> search_results = AD()->search(QString(), QList<QString>(), SearchScope_Object, dn);
    const bool exists = (search_results.size() == 1);

    return exists;
}

void TestADMC::tab(const int n) {
    for (int i = 0; i < n; i++) {
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
    }
}

QTEST_MAIN(TestADMC)
