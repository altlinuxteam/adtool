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

#ifndef ATTRIBUTE_MULTI_EDIT_H
#define ATTRIBUTE_MULTI_EDIT_H

#include <QObject>
#include <QList>

class QFormLayout;
class AdInterface;
class PropertiesMultiTab;

/**
 */

class AttributeMultiEdit : public QObject {
Q_OBJECT
public:
    AttributeMultiEdit(QList<AttributeMultiEdit *> *edits_out, QObject *parent);

    virtual void add_to_layout(QFormLayout *layout) = 0;
    virtual bool apply(AdInterface &ad, const QList<QString> &target_list) = 0;
    virtual void reset() = 0;

signals:
    void edited();

private:
    bool m_modified;
};

void multi_edits_connect_to_tab(QList<AttributeMultiEdit *> edits, PropertiesMultiTab *tab);

#endif /* ATTRIBUTE_MULTI_EDIT_H */