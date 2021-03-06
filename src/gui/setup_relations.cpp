/*
 *  The ManaPlus Client
 *  Copyright (C) 2008-2009  The Mana World Development Team
 *  Copyright (C) 2009-2010  The Mana Developers
 *  Copyright (C) 2011-2012  The ManaPlus Developers
 *
 *  This file is part of The ManaPlus Client.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/setup_relations.h"

#include "actorspritemanager.h"
#include "configuration.h"
#include "localplayer.h"
#include "logger.h"

#include "gui/editdialog.h"
#include "gui/okdialog.h"

#include "gui/widgets/button.h"
#include "gui/widgets/checkbox.h"
#include "gui/widgets/dropdown.h"
#include "gui/widgets/label.h"
#include "gui/widgets/layouthelper.h"
#include "gui/widgets/scrollarea.h"
#include "gui/widgets/guitable.h"
#include "gui/widgets/textfield.h"

#include "utils/dtor.h"
#include "utils/gettext.h"

#define COLUMNS_NR 2 // name plus listbox
#define NAME_COLUMN 0
#define RELATION_CHOICE_COLUMN 1

#define ROW_HEIGHT 12
// The following column widths really shouldn't be hardcoded
// but should scale with the size of the widget... except
// that, right now, the widget doesn't exactly scale either.
#define NAME_COLUMN_WIDTH 230
#define RELATION_CHOICE_COLUMN_WIDTH 80

#define WIDGET_AT(row, column) (((row) * COLUMNS_NR) + column)

#include "debug.h"

static const char *table_titles[COLUMNS_NR] =
{
    N_("Name"),
    N_("Relation")
};

static const char *RELATION_NAMES[PlayerRelation::RELATIONS_NR] =
{
    N_("Neutral"),
    N_("Friend"),
    N_("Disregarded"),
    N_("Ignored"),
    N_("Erased"),
    N_("Blacklisted"),
    N_("Enemy")
};

class PlayerRelationListModel : public gcn::ListModel
{
public:
    virtual ~PlayerRelationListModel()
    { }

    virtual int getNumberOfElements()
    {
        return PlayerRelation::RELATIONS_NR;
    }

    virtual std::string getElementAt(int i)
    {
        if (i >= getNumberOfElements() || i < 0)
            return "";
        return gettext(RELATION_NAMES[i]);
    }
};

class PlayerTableModel : public TableModel
{
public:
    PlayerTableModel() :
        mPlayers(nullptr),
        mListModel(new PlayerRelationListModel)
    {
        playerRelationsUpdated();
    }

    virtual ~PlayerTableModel()
    {
        freeWidgets();
        delete mListModel;
        mListModel = nullptr;
        delete mPlayers;
        mPlayers = nullptr;
    }

    virtual int getRows() const
    {
        if (mPlayers)
            return static_cast<int>(mPlayers->size());
        else
            return 0;
    }

    virtual int getColumns() const
    {
        return COLUMNS_NR;
    }

    virtual int getRowHeight() const
    {
        return ROW_HEIGHT;
    }

    virtual int getColumnWidth(int index) const
    {
        if (index == NAME_COLUMN)
            return NAME_COLUMN_WIDTH;
        else
            return RELATION_CHOICE_COLUMN_WIDTH;
    }

    virtual void playerRelationsUpdated()
    {
        signalBeforeUpdate();

        freeWidgets();
        StringVect *player_names = player_relations.getPlayers();

        if (!player_names)
            return;

        delete mPlayers;
        mPlayers = player_names;

        // set up widgets
        for (unsigned int r = 0; r < player_names->size(); ++r)
        {
            std::string name = (*player_names)[r];
            gcn::Widget *widget = new Label(name);
            mWidgets.push_back(widget);

            gcn::DropDown *choicebox = new DropDown(mListModel);
            choicebox->setSelected(player_relations.getRelation(name));
            mWidgets.push_back(choicebox);
        }

        signalAfterUpdate();
    }

    virtual void updateModelInRow(int row)
    {
        gcn::DropDown *choicebox = static_cast<gcn::DropDown *>(
                                   getElementAt(row, RELATION_CHOICE_COLUMN));
        player_relations.setRelation(getPlayerAt(row),
                                   static_cast<PlayerRelation::Relation>(
                                   choicebox->getSelected()));
    }


    virtual gcn::Widget *getElementAt(int row, int column) const
    {
        return mWidgets[WIDGET_AT(row, column)];
    }

    virtual void freeWidgets()
    {
        delete mPlayers;
        mPlayers = nullptr;

        delete_all(mWidgets);
        mWidgets.clear();
    }

    std::string getPlayerAt(int index) const
    {
        if (index < 0 || index >= static_cast<signed>(mPlayers->size()))
            return "";
        return (*mPlayers)[index];
    }

protected:
    StringVect *mPlayers;
    std::vector<gcn::Widget *> mWidgets;
    PlayerRelationListModel *mListModel;
};

/**
 * Class for choosing one of the various `what to do when ignoring a player' options
 */
class IgnoreChoicesListModel : public gcn::ListModel
{
public:
    virtual ~IgnoreChoicesListModel()
    { }

    virtual int getNumberOfElements()
    {
        return static_cast<int>(player_relations.getPlayerIgnoreStrategies()
                                ->size());
    }

    virtual std::string getElementAt(int i)
    {
        if (i >= getNumberOfElements() || i < 0)
            return _("???");

        return (*player_relations.getPlayerIgnoreStrategies())
            [i]->mDescription;
    }
};

#define ACTION_DELETE "delete"
#define ACTION_TABLE "table"
#define ACTION_STRATEGY "strategy"

Setup_Relations::Setup_Relations():
    mPlayerTableTitleModel(new StaticTableModel(1, COLUMNS_NR)),
    mPlayerTableModel(new PlayerTableModel),
    mPlayerTable(new GuiTable(mPlayerTableModel)),
    mPlayerTitleTable(new GuiTable(mPlayerTableTitleModel)),
    mPlayerScrollArea(new ScrollArea(mPlayerTable)),
    mDefaultTrading(new CheckBox(_("Allow trading"),
                player_relations.getDefault() & PlayerRelation::TRADE)),
    mDefaultWhisper(new CheckBox(_("Allow whispers"),
                player_relations.getDefault() & PlayerRelation::WHISPER)),
    mDeleteButton(new Button(_("Delete"), ACTION_DELETE, this))
{
    setName(_("Relations"));

    mPlayerTable->setOpaque(false);

    mPlayerTableTitleModel->fixColumnWidth(NAME_COLUMN, NAME_COLUMN_WIDTH);
    mPlayerTableTitleModel->fixColumnWidth(RELATION_CHOICE_COLUMN,
                                           RELATION_CHOICE_COLUMN_WIDTH);
    mPlayerTitleTable->setBackgroundColor(gcn::Color(0xbf, 0xbf, 0xbf));

    mIgnoreActionChoicesModel = new IgnoreChoicesListModel;
    mIgnoreActionChoicesBox = new DropDown(mIgnoreActionChoicesModel);

    for (int i = 0; i < COLUMNS_NR; i++)
        mPlayerTableTitleModel->set(0, i, new Label(gettext(table_titles[i])));

    mPlayerTitleTable->setLinewiseSelection(true);

    mPlayerScrollArea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
    mPlayerTable->setActionEventId(ACTION_TABLE);
    mPlayerTable->setLinewiseSelection(true);
    mPlayerTable->addActionListener(this);

    gcn::Label *ignore_action_label = new Label(_("When ignoring:"));

    mIgnoreActionChoicesBox->setActionEventId(ACTION_STRATEGY);
    mIgnoreActionChoicesBox->addActionListener(this);

    int ignore_strategy_index = 0; // safe default

    if (player_relations.getPlayerIgnoreStrategy())
    {
        ignore_strategy_index = player_relations.getPlayerIgnoreStrategyIndex(
            player_relations.getPlayerIgnoreStrategy()->mShortName);
        if (ignore_strategy_index < 0)
            ignore_strategy_index = 0;
    }
    mIgnoreActionChoicesBox->setSelected(ignore_strategy_index);
    mIgnoreActionChoicesBox->adjustHeight();

    reset();

    // Do the layout
    LayoutHelper h(this);
    ContainerPlacer place = h.getPlacer(0, 0);

    place(0, 0, mPlayerTitleTable, 6);
    place(0, 1, mPlayerScrollArea, 6, 4).setPadding(2);
    place(0, 5, mDeleteButton);
    place(3, 5, ignore_action_label, 1);
    place(4, 5, mIgnoreActionChoicesBox, 2).setPadding(2);
    place(3, 6, mDefaultTrading, 3);
    place(3, 7, mDefaultWhisper, 3);

    player_relations.addListener(this);

    setDimension(gcn::Rectangle(0, 0, 500, 350));
}

Setup_Relations::~Setup_Relations()
{
    player_relations.removeListener(this);
    delete mIgnoreActionChoicesModel;
    mIgnoreActionChoicesModel = nullptr;
}


void Setup_Relations::reset()
{
    // We now have to search through the list of ignore choices to find the
    // current selection. We could use an index into the table of config
    // options in player_relations instead of strategies to sidestep this.
    int selection = 0;
    for (unsigned int i = 0;
                      i < player_relations.getPlayerIgnoreStrategies()->size();
                      ++i)
        if ((*player_relations.getPlayerIgnoreStrategies())[i] ==
            player_relations.getPlayerIgnoreStrategy())
        {

            selection = i;
            break;
        }

    mIgnoreActionChoicesBox->setSelected(selection);
}

void Setup_Relations::apply()
{
    player_relations.store();

    unsigned int old_default_relations = player_relations.getDefault() &
                                         ~(PlayerRelation::TRADE |
                                           PlayerRelation::WHISPER);
    player_relations.setDefault(old_default_relations
                                | (mDefaultTrading->isSelected() ?
                                       PlayerRelation::TRADE : 0)
                                | (mDefaultWhisper->isSelected() ?
                                       PlayerRelation::WHISPER : 0));

    if (actorSpriteManager)
        actorSpriteManager->updatePlayerNames();

    if (player_node)
        player_node->setCheckNameSetting(true);
}

void Setup_Relations::cancel()
{
}

void Setup_Relations::action(const gcn::ActionEvent &event)
{
    if (event.getId() == ACTION_TABLE)
    {
        // temporarily eliminate ourselves: we are fully aware of this change,
        // so there is no need for asynchronous updates.  (In fact, thouse
        // might destroy the widet that triggered them, which would be rather
        // embarrassing.)
        player_relations.removeListener(this);

        int row = mPlayerTable->getSelectedRow();
        if (row >= 0)
            mPlayerTableModel->updateModelInRow(row);

        player_relations.addListener(this);

    }
    else if (event.getId() == ACTION_DELETE)
    {
        int player_index = mPlayerTable->getSelectedRow();

        if (player_index < 0)
            return;

        std::string name = mPlayerTableModel->getPlayerAt(player_index);

        player_relations.removePlayer(name);
    }
    else if (event.getId() == ACTION_STRATEGY)
    {
        PlayerIgnoreStrategy *s =
            (*player_relations.getPlayerIgnoreStrategies())[
                mIgnoreActionChoicesBox->getSelected()];

        player_relations.setPlayerIgnoreStrategy(s);
    }
}

void Setup_Relations::updatedPlayer(const std::string &name A_UNUSED)
{
    mPlayerTableModel->playerRelationsUpdated();
    mDefaultTrading->setSelected(
            player_relations.getDefault() & PlayerRelation::TRADE);
    mDefaultWhisper->setSelected(
            player_relations.getDefault() & PlayerRelation::WHISPER);
    if (player_node)
        player_node->updateName();
}

void Setup_Relations::updateAll()
{
    PlayerTableModel *model = new PlayerTableModel();
    mPlayerTable->setModel(model);
    delete mPlayerTableModel;
    mPlayerTableModel = model;
    int ignore_strategy_index = 0; // safe default

    if (player_relations.getPlayerIgnoreStrategy())
    {
        ignore_strategy_index = player_relations.getPlayerIgnoreStrategyIndex(
                player_relations.getPlayerIgnoreStrategy()->mShortName);
        if (ignore_strategy_index < 0)
            ignore_strategy_index = 0;
    }
    mIgnoreActionChoicesBox->setSelected(ignore_strategy_index);
    mIgnoreActionChoicesBox->adjustHeight();
    reset();
}
void Setup_Relations::externalUpdated()
{
    mDefaultTrading->setSelected(
        player_relations.getDefault() & PlayerRelation::TRADE);
    mDefaultWhisper->setSelected(
        player_relations.getDefault() & PlayerRelation::WHISPER);
}
