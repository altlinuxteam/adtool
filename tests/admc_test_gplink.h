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

#ifndef ADMC_TEST_GPLINK_H
#define ADMC_TEST_GPLINK_H

#include <QObject>

#include <QTest>

class ADMCTestGplink : public QObject {
Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void test_to_string();
    void test_get_gpos();
    void test_add();
    void test_remove();
    void test_move_up();
    void test_move_down();
    void test_get_option();
    void test_set_option();
    void test_equals();
};

#endif /* ADMC_TEST_GPLINK_H */