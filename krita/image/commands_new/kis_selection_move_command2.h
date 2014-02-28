/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KIS_SELECTION_MOVE_COMMAND2_H
#define KIS_SELECTION_MOVE_COMMAND2_H

#include "kis_move_command_common.h"

/**
 * A specialized class for moving a selection without its flattening and recalculation
 * of the outline cache. See details in a comment to KisMoveCommandCommon.
 */
class KRITAIMAGE_EXPORT KisSelectionMoveCommand2 : public KisMoveCommandCommon<KisSelectionSP>
{
public:
    KisSelectionMoveCommand2(KisSelectionSP object, const QPoint& oldPos, const QPoint& newPos, KUndo2Command *parent = 0)
        : KisMoveCommandCommon<KisSelectionSP>(object, oldPos, newPos, parent)
    {
    }

    void redo() {
        KisMoveCommandCommon<KisSelectionSP>::redo();
        m_object->notifySelectionChanged();
    }

    void undo() {
        KisMoveCommandCommon<KisSelectionSP>::undo();
        m_object->notifySelectionChanged();
    }
};

#endif