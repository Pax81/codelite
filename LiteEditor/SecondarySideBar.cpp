#include "SecondarySideBar.hpp"

#include "SideBar.hpp"
#include "cl_config.h"
#include "editor_config.h"
#include "event_notifier.h"
#include "globals.h"
#include "imanager.h"
#include "wxTerminalCtrl/clBuiltinTerminalPane.hpp"

#include <wx/xrc/xmlres.h>

#define VIEW_NAME "Secondary Sidebar"

SecondarySideBar::SecondarySideBar(wxWindow* parent, long style)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    m_book = new clSideBarCtrl(this);
    GetSizer()->Add(m_book, wxSizerFlags().Expand().Proportion(1));

    auto direction = EditorConfigST::Get()->GetOptions()->GetWorkspaceTabsDirection();
    m_book->SetButtonPosition(direction);

    GetSizer()->Fit(this);
    m_book->Bind(wxEVT_SIDEBAR_CONTEXT_MENU, &SecondarySideBar::OnContextMenu, this);
    EventNotifier::Get()->Bind(wxEVT_EDITOR_CONFIG_CHANGED, &SecondarySideBar::OnSettingsChanged, this);
}

SecondarySideBar::~SecondarySideBar()
{
    wxArrayString tabs;
    tabs.reserve(m_book->GetPageCount());
    for (size_t i = 0; i < m_book->GetPageCount(); ++i) {
        if (m_book->GetPage(i) == m_terminal_pane) {
            // The terminal page is not part of "secondary_side_bar.tabs": it is restored via
            // "terminal.in_secondary_sidebar"
            continue;
        }
        tabs.Add(m_book->GetPageText(i));
    }

    EventNotifier::Get()->Unbind(wxEVT_EDITOR_CONFIG_CHANGED, &SecondarySideBar::OnSettingsChanged, this);

    // override the values
    clConfig::Get().Write("secondary_side_bar.tabs", tabs);
    clConfig::Get().Write("secondary_side_bar.selection", m_book->GetSelection());
}

void SecondarySideBar::SetSideBar(SideBar* sb) { m_sidebar = sb; }

void SecondarySideBar::AddPage(wxWindow* win, const wxString& bmpname, const wxString& label)
{
    m_book->AddPage(win, label, bmpname, true);
    m_book->Realize();
    m_book->GetSizer()->Layout();

    auto secondary_tabs = clConfig::Get().Read("secondary_side_bar.tabs", wxArrayString{});
    if (secondary_tabs.Index(label) == wxNOT_FOUND) {
        secondary_tabs.Add(label);
        clConfig::Get().Write("secondary_side_bar.tabs", secondary_tabs);
    }
}

void SecondarySideBar::ShowTerminalPane(bool takeFocus)
{
    if (m_terminal_pane == nullptr) {
        // A tab dragged into another notebook would lose its event handling here, and the pane must not show the
        // Terminal tab of the bottom Output View when it gets a new terminal
        m_terminal_pane = new clBuiltinTerminalPane(this,
                                                    wxID_ANY,
                                                    /*allowNotebookDrag=*/false,
                                                    /*showTerminalTabInOutputPane=*/false);
        // Not AddPage(): that writes the label into "secondary_side_bar.tabs", which must not contain the terminal
        m_book->AddPage(m_terminal_pane, kTerminalPageLabel, "console", true);
        m_book->Realize();
        m_book->GetSizer()->Layout();
    }

    int pos = m_book->GetPageIndex(m_terminal_pane);
    if (pos != wxNOT_FOUND) {
        m_book->SetSelection(pos);
    }
    clGetManager()->ShowPane(VIEW_NAME, true);

    if (takeFocus) {
        m_terminal_pane->CallAfter(&clBuiltinTerminalPane::SetFocus);
    }
}

void SecondarySideBar::OnContextMenu(clContextMenuEvent& event)
{
    wxMenu menu;
    int pos = event.GetSelection();
    if (pos != wxNOT_FOUND && m_book->GetPage(pos) == m_terminal_pane) {
        // The terminal lives only here: it cannot move to the primary sidebar
        menu.Append(XRCID("sidebar-remove-terminal"), _("Remove from secondary side bar"));
        menu.Bind(
            wxEVT_MENU,
            [pos, this](wxCommandEvent& e) {
                wxUnusedVar(e);
                RemoveTerminalPage(pos);
            },
            XRCID("sidebar-remove-terminal"));
    } else {
        menu.Append(XRCID("sidebar-move-to-sidebar"), _("Move to primary sidebar"));
        menu.Bind(
            wxEVT_MENU,
            [pos, this](wxCommandEvent& e) {
                wxUnusedVar(e);
                MoveToPrimarySideBar(pos);
            },
            XRCID("sidebar-move-to-sidebar"));
    }
    m_book->PopupMenu(&menu);
}

void SecondarySideBar::RemoveTerminalPage(int pos)
{
    // DeletePage() deletes the pane, which terminates its shell processes
    m_book->DeletePage(pos);
    m_terminal_pane = nullptr;
    m_book->GetSizer()->Layout();

    clConfig::Get().Write("terminal.in_secondary_sidebar", false);

    if (m_book->GetPageCount() == 0) {
        // No more pages -> hide this view
        clGetManager()->ShowPane(VIEW_NAME, false);
    }
}

void SecondarySideBar::MoveToPrimarySideBar(int pos)
{
    wxString label = m_book->GetPageText(pos);
    wxString bmp = m_book->GetPageBitmap(pos);
    wxWindow* win = m_book->GetPage(pos);

    m_book->RemovePage(pos);
    m_book->GetSizer()->Layout();

    // add it to the right side bar
    m_sidebar->AddPage(win, bmp, label);
    m_sidebar->GetNotebook()->Realize();

    if (m_book->GetPageCount() == 0) {
        // No more pages -> hide this view
        clGetManager()->ShowPane(VIEW_NAME, false);
    }
}

void SecondarySideBar::SetSelection(int selection)
{
    if ((size_t)selection >= m_book->GetPageCount()) {
        return;
    }
    m_book->SetSelection(selection);
}

void SecondarySideBar::OnSettingsChanged(wxCommandEvent& event)
{
    event.Skip();

    auto direction = EditorConfigST::Get()->GetOptions()->GetWorkspaceTabsDirection();
    // on the secondary bar, "left" means: vertical tabs, i.e. place them on the right side
    if (direction == wxLEFT) {
        direction = wxRIGHT;
    }
    m_book->SetButtonPosition(direction);
}
