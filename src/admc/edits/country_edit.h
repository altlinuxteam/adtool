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

#ifndef COUNTRY_EDIT_H
#define COUNTRY_EDIT_H

#include "edits/attribute_edit.h"

class QComboBox;

class CountryEdit final : public AttributeEdit {
Q_OBJECT
public:
    CountryEdit(QObject *parent);
    DECL_ATTRIBUTE_EDIT_VIRTUALS();

private:
    int original_value;
    QComboBox *combo;
    
    // NOTE: country codes are 3 digits only, so 0-999 = 1000
    QString country_strings[1000];
    QString country_abbreviations[1000];
};

#endif /* COUNTRY_EDIT_H */