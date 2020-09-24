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

#ifndef DETAILS_TAB_H
#define DETAILS_TAB_H

#include <QWidget>
#include <QString>

class DetailsWidget;

class DetailsTab : public QWidget {
Q_OBJECT

public:
    DetailsTab(DetailsWidget *details_arg);
    
    QString target() const;

    virtual bool accepts_target() const = 0;
    virtual bool changed() const = 0;
    virtual void reload() = 0;
    virtual bool verify() = 0;
    virtual void apply() = 0;

signals:
    void edited();
    
public slots:
    void on_edit_edited();

private:
    DetailsWidget *details;
};

#define DECL_DETAILS_TAB_VIRTUALS()\
bool accepts_target() const;\
bool changed() const;\
void reload();\
bool verify();\
void apply();

#endif /* DETAILS_TAB_H */