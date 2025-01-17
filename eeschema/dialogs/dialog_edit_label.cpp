/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <sch_edit_frame.h>
#include <base_units.h>
#include <sch_validators.h>
#include <tool/tool_manager.h>
#include <general.h>
#include <gr_text.h>
#include <confirm.h>
#include <sch_symbol.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <dialogs/html_messagebox.h>
#include <dialog_edit_label.h>
#include <kicad_string.h>
#include <tool/actions.h>
#include <scintilla_tricks.h>

class SCH_EDIT_FRAME;
class SCH_TEXT;


DIALOG_LABEL_EDITOR::DIALOG_LABEL_EDITOR( SCH_EDIT_FRAME* aParent, SCH_TEXT* aTextItem ) :
    DIALOG_LABEL_EDITOR_BASE( aParent ),
    m_textSize( aParent, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits, false ),
    m_netNameValidator( true ),
    m_scintillaTricks( nullptr ),
    m_helpWindow( nullptr )
{
    m_Parent = aParent;
    m_CurrentText = aTextItem;

    switch( m_CurrentText->Type() )
    {
    case SCH_GLOBAL_LABEL_T:       SetTitle( _( "Global Label Properties" ) );           break;
    case SCH_HIER_LABEL_T:         SetTitle( _( "Hierarchical Label Properties" ) );     break;
    case SCH_LABEL_T:              SetTitle( _( "Label Properties" ) );                  break;
    case SCH_SHEET_PIN_T:          SetTitle( _( "Hierarchical Sheet Pin Properties" ) ); break;
    default:                       SetTitle( _( "Text Properties" ) );                   break;
    }

    m_valueMultiLine->SetEOLMode( wxSTC_EOL_LF );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_valueMultiLine, wxT( "()" ) );

    if( m_CurrentText->IsMultilineAllowed() )
    {
        m_activeTextCtrl = m_valueMultiLine;
        m_activeTextEntry = nullptr;

        m_labelSingleLine->Show( false );
        m_valueSingleLine->Show( false );
        m_labelCombo->Show( false );
        m_valueCombo->Show( false );

        m_textEntrySizer->AddGrowableRow( 0 );
    }
    else if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T || m_CurrentText->Type() == SCH_LABEL_T )
    {
        m_activeTextCtrl = m_valueCombo;
        m_activeTextEntry = m_valueCombo;

        m_labelSingleLine->Show( false );  m_valueSingleLine->Show( false );
        m_labelMultiLine->Show( false );   m_valueMultiLine->Show( false );

        m_valueCombo->SetValidator( m_netNameValidator );
    }
    else
    {
        m_activeTextCtrl = m_valueSingleLine;
        m_activeTextEntry = m_valueSingleLine;

        m_labelCombo->Show( false );
        m_valueCombo->Show( false );
        m_labelMultiLine->Show( false );
        m_valueMultiLine->Show( false );

        if( m_CurrentText->Type() != SCH_TEXT_T )
            m_valueSingleLine->SetValidator( m_netNameValidator );

        m_valueCombo->SetValidator( m_netNameValidator );
    }

    SetInitialFocus( m_activeTextCtrl );

    m_TextShape->Show( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T ||
                       m_CurrentText->Type() == SCH_HIER_LABEL_T );

    if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T )
    {
        wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
        infoFont.SetSymbolicSize( wxFONTSIZE_X_SMALL );
        m_note1->SetFont( infoFont );
        m_note2->SetFont( infoFont );
    }
    else
    {
        m_note1->Show( false );
        m_note2->Show( false );
    }

    m_sdbSizer1OK->SetDefault();
    Layout();

    m_valueMultiLine->Bind( wxEVT_STC_CHARADDED, &DIALOG_LABEL_EDITOR::onScintillaCharAdded, this );

    int    size = wxNORMAL_FONT->GetPointSize();
    wxFont fixedFont( size, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL );

    for( size_t i = 0; i < wxSTC_STYLE_MAX; ++i )
        m_valueMultiLine->StyleSetFont( i, fixedFont );

    // Addresses a bug in wx3.0 where styles are not correctly set
    m_valueMultiLine->StyleClearAll();

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient because the
    // various versions have different controls so we want to store sizes for each version.
    m_hash_key = TO_UTF8( GetTitle() );


    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_LABEL_EDITOR::~DIALOG_LABEL_EDITOR()
{
    delete m_scintillaTricks;

    if( m_helpWindow )
        m_helpWindow->Destroy();
}


bool DIALOG_LABEL_EDITOR::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( m_CurrentText->Type() == SCH_TEXT_T )
    {
        SCHEMATIC& schematic = m_Parent->Schematic();

        // show text variable cross-references in a human-readable format
        m_valueMultiLine->SetValue( schematic.ConvertKIIDsToRefs( m_CurrentText->GetText() ) );
    }
    else
    {
        // show control characters in a human-readable format
        m_activeTextEntry->SetValue( UnescapeString( m_CurrentText->GetText() ) );
    }

    if( m_valueCombo->IsShown() )
    {
        // Load the combobox with the existing labels of the same type
        std::set<wxString> existingLabels;
        SCH_SCREENS        allScreens( m_Parent->Schematic().Root() );

        for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
        {
            for( SCH_ITEM* item : screen->Items().OfType( m_CurrentText->Type() ) )
            {
                auto textItem = static_cast<const SCH_TEXT*>( item );
                existingLabels.insert( UnescapeString( textItem->GetText() ) );
            }
        }

        wxArrayString existingLabelArray;

        for( const wxString& label : existingLabels )
            existingLabelArray.push_back( label );

        // existingLabelArray.Sort();
        m_valueCombo->Append( existingLabelArray );
    }

    // Set text options:
    m_TextOrient->SetSelection( static_cast<int>( m_CurrentText->GetLabelSpinStyle() ) );

    m_TextShape->SetSelection( static_cast<int>( m_CurrentText->GetShape() ) );

    int style = 0;

    if( m_CurrentText->IsItalic() )
        style = 1;

    if( m_CurrentText->IsBold() )
        style += 2;

    m_TextStyle->SetSelection( style );

    m_textSize.SetValue( m_CurrentText->GetTextWidth() );

    return true;
}


/*!
 * wxEVT_COMMAND_ENTER event handler for single-line control
 */
void DIALOG_LABEL_EDITOR::OnEnterKey( wxCommandEvent& aEvent )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


void DIALOG_LABEL_EDITOR::onScintillaCharAdded( wxStyledTextEvent &aEvent )
{
    wxStyledTextCtrl* te = m_valueMultiLine;
    wxArrayString     autocompleteTokens;
    int               text_pos = te->GetCurrentPos();
    int               start = te->WordStartPosition( text_pos, true );
    wxString          partial;

    auto textVarRef =
            [&]( int pos )
            {
                return pos >= 2 && te->GetCharAt( pos-2 ) == '$' && te->GetCharAt( pos-1 ) == '{';
            };

    // Check for cross-reference
    if( start > 1 && te->GetCharAt( start-1 ) == ':' )
    {
        int refStart = te->WordStartPosition( start-1, true );

        if( textVarRef( refStart ) )
        {
            partial = te->GetRange( start+1, text_pos );

            wxString           ref = te->GetRange( refStart, start-1 );
            SCH_SHEET_LIST     sheets = m_Parent->Schematic().GetSheets();
            SCH_REFERENCE_LIST refs;
            SCH_COMPONENT*     refSymbol = nullptr;

            sheets.GetSymbols( refs );

            for( size_t jj = 0; jj < refs.GetCount(); jj++ )
            {
                if( refs[ jj ].GetSymbol()->GetRef( &refs[ jj ].GetSheetPath(), true ) == ref )
                {
                    refSymbol = refs[ jj ].GetSymbol();
                    break;
                }
            }

            if( refSymbol )
                refSymbol->GetContextualTextVars( &autocompleteTokens );
        }
    }
    else if( textVarRef( start ) )
    {
        partial = te->GetTextRange( start, text_pos );

        m_CurrentText->GetContextualTextVars( &autocompleteTokens );

        SCHEMATIC* schematic = m_CurrentText->Schematic();

        if( schematic && schematic->CurrentSheet().Last() )
            schematic->CurrentSheet().Last()->GetContextualTextVars( &autocompleteTokens );

        for( std::pair<wxString, wxString> entry : Prj().GetTextVars() )
            autocompleteTokens.push_back( entry.first );
    }

    m_scintillaTricks->DoAutocomplete( partial, autocompleteTokens );
    m_valueMultiLine->SetFocus();
}


bool DIALOG_LABEL_EDITOR::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Don't allow text to disappear; it can be difficult to correct if you can't select it
    if( !m_textSize.Validate( 0.01, 1000.0, EDA_UNITS::MILLIMETRES ) )
        return false;

    wxString text;

    /* save old text in undo list if not already in edit */
    if( m_CurrentText->GetEditFlags() == 0 )
        m_Parent->SaveCopyInUndoList( m_Parent->GetScreen(), m_CurrentText, UNDO_REDO::CHANGED, false );

    m_Parent->GetCanvas()->Refresh();

    if( m_CurrentText->Type() == SCH_TEXT_T )
    {
        // convert any text variable cross-references to their UUIDs
        text = m_Parent->Schematic().ConvertRefsToKIIDs( m_valueMultiLine->GetValue() );
    }
    else
    {
        // labels need escaping
        text = EscapeString( m_activeTextEntry->GetValue(), CTX_NETNAME );
    }

    if( !text.IsEmpty() )
    {
        m_CurrentText->SetText( text );
    }
    else if( !m_CurrentText->IsNew() )
    {
        DisplayError( this, _( "Label requires non-empty text." ) );
        return false;
    }

    m_CurrentText->SetLabelSpinStyle( (LABEL_SPIN_STYLE::SPIN) m_TextOrient->GetSelection() );

    m_CurrentText->SetTextSize( wxSize( m_textSize.GetValue(), m_textSize.GetValue() ) );

    if( m_TextShape )
        m_CurrentText->SetShape( (PINSHEETLABEL_SHAPE) m_TextShape->GetSelection() );

    int style = m_TextStyle->GetSelection();

    m_CurrentText->SetItalic( ( style & 1 ) );

    if( ( style & 2 ) )
    {
        m_CurrentText->SetBold( true );
        m_CurrentText->SetTextThickness( GetPenSizeForBold( m_CurrentText->GetTextWidth() ) );
    }
    else
    {
        m_CurrentText->SetBold( false );
        m_CurrentText->SetTextThickness( 0 );    // Use default pen width
    }

    m_Parent->UpdateItem( m_CurrentText );
    m_Parent->GetCanvas()->Refresh();
    m_Parent->OnModify();

    if( m_CurrentText->Type() == SCH_GLOBAL_LABEL_T )
    {
        SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( m_CurrentText );
        label->UpdateIntersheetRefProps();
    }

    return true;
}


void DIALOG_LABEL_EDITOR::OnFormattingHelp( wxHyperlinkEvent& aEvent )
{
    m_helpWindow = SCH_TEXT::ShowSyntaxHelp( this );
}
