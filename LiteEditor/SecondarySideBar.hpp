#pragma once

// clang-format off
#include "cl_command_event.h"
#include "clSideBarCtrl.hpp"
// clang-format on

class SideBar;
class clBuiltinTerminalPane;
class SecondarySideBar : public wxPanel
{
    /// The label of the terminal page. It is kept out of "secondary_side_bar.tabs" (the terminal has its own flag:
    /// "terminal.in_secondary_sidebar"), so it must never be written to that list
    static constexpr const char kTerminalPageLabel[] = "Terminal";

protected:
    void OnContextMenu(clContextMenuEvent& event);
    void MoveToPrimarySideBar(int pos);
    void RemoveTerminalPage(int pos);
    void OnSettingsChanged(wxCommandEvent& event);

public:
    SecondarySideBar(wxWindow* parent, long style = wxBORDER_NONE);
    ~SecondarySideBar();
    const char* GetCaption() const { return "Secondary Sidebar"; }
    void SetSideBar(SideBar* sb);
    void AddPage(wxWindow* win, const wxString& bmpname, const wxString& label);
    void SetSelection(int selection);
    clSideBarCtrl* GetNotebook() { return m_book; }
    SideBar* GetSidebar() { return m_sidebar; }

    /// The terminal pane of this side bar, or nullptr when there is no terminal page
    clBuiltinTerminalPane* GetTerminalPane() { return m_terminal_pane; }
    bool HasTerminalPane() const { return m_terminal_pane != nullptr; }

    /**
     * @brief Show the terminal page, creating the terminal pane on the first call.
     * Also shows the "Secondary Sidebar" pane and selects the page.
     * @param takeFocus Pass the focus to the terminal (if it has one)
     */
    void ShowTerminalPane(bool takeFocus);

private:
    clSideBarCtrl* m_book{nullptr};
    SideBar* m_sidebar{nullptr};
    clBuiltinTerminalPane* m_terminal_pane{nullptr};
};
