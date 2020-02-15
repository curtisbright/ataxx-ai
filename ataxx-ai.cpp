#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/textfile.h"
#include "masks.h"
#include <stdio.h>
#define max(x,y) (x > y ? x : y)

wxListCtrl* MoveList;
wxMenu* gamemenu;
wxMenu* bluemenu;
wxMenu* greenmenu;
wxStatusBar* statusbar;
int count = 0;
int maxcount = 0;

// Menu id constants
enum {
	ID_NEW_GAME = wxID_HIGHEST+1,
	ID_AI_MOVE,
	ID_EDIT_MODE,
	ID_SAVE_GAME,
	ID_LOAD_GAME,
	ID_BLUE_OFF,
	ID_BLUE_1,
	ID_BLUE_2,
	ID_BLUE_3,
	ID_BLUE_4,
	ID_BLUE_5,
	ID_BLUE_6,
	ID_BLUE_7,
	ID_BLUE_8,
	ID_BLUE_9,
	ID_GREEN_OFF,
	ID_GREEN_1,
	ID_GREEN_2,
	ID_GREEN_3,
	ID_GREEN_4,
	ID_GREEN_5,
	ID_GREEN_6,
	ID_GREEN_7,
	ID_GREEN_8,
	ID_GREEN_9
};

// Return the number of set bits in board
int score(longint board)
{	int score;
	for(score=0; board; board&=board-1)
		score++;
	return score;
}

// Negamax algorithm with alpha-beta pruning
// Input: Given the game state, depth to search, and alpha/beta bounds
// Output: return the utility of the state,
// and put the state of the optimal move found in the memory locations pointed to by (optimalboard, optimalactive)
int negamax(longint board, longint active, int depth, int alpha, int beta, longint* optimalboard, longint* optimalactive)
{	longint* boards = NULL;
	longint* actives = NULL;
	int num = 0;
	int i;
	int maxi=-1;
	int tempscore;
	int maxscore=-50;
	longint clonemoves = 0;
	longint tempboard;
	longint jumpmoves;

	board &= active;

	// Leaf node
	if(depth==0)
	{
		return score(board)-score((~board)&active);
	}

	// Find all possible clone moves

	for(tempboard=board; tempboard; tempboard&=tempboard-1)
		clonemoves |= clonemask[ilog2(tempboard&-tempboard)];

	for(clonemoves&=~active; clonemoves; clonemoves&=clonemoves-1)
	{	boards = (longint*)realloc(boards, (num+1)*sizeof(longint));
		actives = (longint*)realloc(actives, (num+1)*sizeof(longint));

		boards[num] = board | (clonemoves&-clonemoves) | clonemask[ilog2(clonemoves&-clonemoves)];
		actives[num] = active | (clonemoves&-clonemoves);

		num++;
	}

	// Find all possible jump moves
	for(tempboard=board; tempboard; tempboard&=tempboard-1)
	{	jumpmoves = jumpmask[ilog2(tempboard&-tempboard)];
		for(jumpmoves&=~active; jumpmoves; jumpmoves&=jumpmoves-1)
		{	boards = (longint*)realloc(boards, (num+1)*sizeof(longint));
			actives = (longint*)realloc(actives, (num+1)*sizeof(longint));

			boards[num] = board | (jumpmoves&-jumpmoves) | clonemask[ilog2(jumpmoves&-jumpmoves)];
			actives[num] = (active | (jumpmoves&-jumpmoves)) & ~(tempboard&-tempboard);

			num++;
		}
	}

	// If no moves possible, pass
	if(num==0)
	{	boards = (longint*)realloc(boards, sizeof(longint));
		actives = (longint*)realloc(actives, sizeof(longint));

		boards[0] = board;
		actives[0] = active;
		num++;
	}

	// Run search through all nodes
	for(i=0; i<num; i++)
	{	
		tempscore = -negamax(~boards[i], actives[i], depth-1, -beta, -alpha, optimalboard, optimalactive);

		if(tempscore > maxscore)
		{	maxscore = tempscore;
			alpha = max(alpha, tempscore);
			maxi = i;
		}
		if(alpha>=beta)
			break;
	}

	*optimalboard = boards[maxi];
	*optimalactive = actives[maxi];
	free(boards);
	free(actives);

	return alpha;
}

int countmoves(longint cells, longint active)
{	int i, j;
	int nummoves = 0;
	longint clonemoves = 0;
	longint jumpmoves;

	cells &= active;
	
	for(i=0; i<49; i++)
		if(cells & mask[i])
			clonemoves |= clonemask[i];

	clonemoves &= ~active;
	for(i=0; i<49; i++)
		if(clonemoves & mask[i])
			nummoves++;

	for(i=0; i<49; i++)
		if(cells & mask[i])
		{	jumpmoves = jumpmask[i] & ~active;
			for(j=0; j<49; j++)
				if(jumpmoves & mask[j])
					nummoves++;
		}

	return nummoves;
}

class MainFrame : public wxFrame
{	
private:
	
	void Reset(wxCommandEvent& event);
	void SetEdit(wxCommandEvent& event);
	void Write(wxCommandEvent& event);
	void Load(wxCommandEvent& event);

public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style);
	void RunAI(wxCommandEvent& event);
	void Undo();
	void Redo();
};

class MainPanel : public wxPanel
{
public:
	int selected;
	longint board;
	longint active;
	longint lastactive;
	char player;

	void LMouseUp(wxMouseEvent& event);
	void RMouseUp(wxMouseEvent& event);
	void Paint(wxPaintEvent& evt);
	void DrawBoard(wxDC& dc);
	void KeyDown(wxKeyEvent& event);
	
	int start;
	MainFrame* par;
	
	MainPanel(MainFrame* parent);
};

MainPanel* panel;

MainPanel::MainPanel(MainFrame* parent) : wxPanel(parent, wxID_ANY, wxPoint(0, 0), wxSize(224, 224), wxTAB_TRAVERSAL, _T("panel"))
{	selected = -1;
	board = STARTBOARD;
	active = STARTACTIVE;
	lastactive = STARTACTIVE;
	player = 'X';
	
	par = parent;

	/*negamax(board, active, DEPTH, -50, 50, &board, &active);
	board = ~board;

	player = (player=='X' ? 'O' : 'X');*/
	
	start = 1;
	
	Connect(wxEVT_PAINT, wxPaintEventHandler(MainPanel::Paint));
	Connect(wxEVT_LEFT_UP, wxMouseEventHandler(MainPanel::LMouseUp));
	Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(MainPanel::RMouseUp));
	Connect(wxEVT_CHAR_HOOK, wxKeyEventHandler(MainPanel::KeyDown));
}

void MainPanel::KeyDown(wxKeyEvent& event)
{	if(event.GetKeyCode()==WXK_LEFT)
		par->Undo();
	else if(event.GetKeyCode()==WXK_RIGHT)
		par->Redo();
}

void MainPanel::RMouseUp(wxMouseEvent& event)
{	int x, y;
	x = event.GetPosition().x/32;
	y = event.GetPosition().y/32;
	wxClientDC dc(this);

	if(start==0)
	{	if(~active & mask[7*x+y])
		{	active |= mask[7*x+y];
			board |= mask[7*x+y];
		}
		else if(active & mask[7*x+y] & board)
		{	board &= ~mask[7*x+y];
		}
		else if(active & mask[7*x+y] & ~board)
		{	board &= ~mask[7*x+y];
			active &= ~mask[7*x+y];
		}
		lastactive = active;
		DrawBoard(dc);
		return;
	}
}

void strmove(longint active, longint lastactive, char* output)
{	int i, j;
	if((active^lastactive)&(~active))
	{	for(i=0; i<7; i++)
			for(j=0; j<7; j++)
			{	if((~active) & lastactive & mask[7*i+j])
				{	output[0] = 'a'+i;
					output[1] = '1'+j;
				}
				if(active & (~lastactive) & mask[7*i+j])
				{	output[2] = 'a'+i;
					output[3] = '1'+j;
				}
			}
	}
	else if((active^lastactive)&(active))
	{	for(i=0; i<7; i++)
			for(j=0; j<7; j++)
			{	if(active & (~lastactive) & mask[7*i+j])
				{	output[0] = 'a'+i;
					output[1] = '1'+j;
				}
			}
	}
	else
	{	strcpy(output, "pass");
	}
}

void MainPanel::LMouseUp(wxMouseEvent& event)
{	int x, y;
	x = event.GetPosition().x/32;
	y = event.GetPosition().y/32;
	wxClientDC dc(this);

	//if(x>6 || y>6)
	//	return;
		
	if(start==0)
	{	if(~active & mask[7*x+y])
		{	active |= mask[7*x+y];
			board &= ~mask[7*x+y];
		}
		else if(active & mask[7*x+y] & ~board)
		{	board |= mask[7*x+y];
		}
		else if(active & mask[7*x+y] & board)
		{	board &= ~mask[7*x+y];
			active &= ~mask[7*x+y];
		}
		lastactive = active;
		DrawBoard(dc);
		return;
	}

	if(active==FULLBOARD || (board&active)==0 || ((~board)&active)==0)
		return;

	if(selected>=0 && (~active & mask[7*x+y]) && (mask[selected] & (clonemask[7*x+y] | jumpmask[7*x+y])))
	{	board |= mask[7*x+y] | clonemask[7*x+y];
		lastactive = active;
		active |= mask[7*x+y];
		if(!(mask[7*x+y] & clonemask[selected]))
			active &= ~mask[selected];
		board = ~board;
		player = (player=='X' ? 'O' : 'X');
		selected = -1;

		char* str = (char*)calloc(5, sizeof(char));
		strmove(active, lastactive, str);
		/*if(count%2==0)
			MoveList->InsertItem(count/2, str);
		else
			MoveList->SetItem(count/2, 1, str);*/
		if(count%2==0)
		{	

			long itemIndex = -1;
		 	while(true)
			{	itemIndex = MoveList->GetNextItem(itemIndex);
		 		if(itemIndex==-1)
					break;
		 		if(itemIndex>=count/2)
				{	MoveList->DeleteItem(itemIndex);
					itemIndex = -1;
				}
			}

			MoveList->InsertItem(count/2, wxString::FromAscii(str));
		}
		else
		{	

			long itemIndex = -1;
		 	while(true)
			{	itemIndex = MoveList->GetNextItem(itemIndex);
		 		if(itemIndex==-1)
					break;
		 		if(itemIndex>count/2)
				{	MoveList->DeleteItem(itemIndex);
					itemIndex = -1;
				}
			}

			MoveList->SetItem(count/2, 1, wxString::FromAscii(str));
		}
		count++;
		maxcount = count;
		free(str);

		DrawBoard(dc);
		lastactive = active;
		/*negamax(board, active, DEPTH1, -50, 50, &board, &active);
		DrawBoard(dc);

		board = ~board;
		player = (player=='X' ? 'O' : 'X');*/

		while(countmoves(board, active)==0 && active!=FULLBOARD)
		{	board = ~board;
			player = (player=='X' ? 'O' : 'X');
			DrawBoard(dc);
			lastactive = active;
			
			if(count%2==0)
				MoveList->InsertItem(count/2, _T("pass"));
			else
				MoveList->SetItem(count/2, 1, _T("pass"));
			
			count++;
			maxcount = count;
			
			/*negamax(board, active, DEPTH1, -50, 50, &board, &active);
			DrawBoard(dc);
			board = ~board;
			player = (player=='X' ? 'O' : 'X');*/
		}

		wxCommandEvent dummy;
		if(player=='X' && !bluemenu->IsChecked(ID_BLUE_OFF))
			par->RunAI(dummy);
		if(player=='O' && !greenmenu->IsChecked(ID_GREEN_OFF))
			par->RunAI(dummy);
	}
	else if((active & mask[7*x+y]) && (board & mask[7*x+y]))
	{	if(selected==7*x+y)
			selected = -1;
		else
			selected = 7*x+y;
		DrawBoard(dc);
	}
}

void MainPanel::DrawBoard(wxDC& dc)
{	int i, j;
	if(player=='O')
		board = ~board;

	dc.Clear();

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);

	for(i=0; i<7; i++)
		for(j=0; j<7; j++)
			dc.DrawRectangle(32*i, 32*j, 32, 32);

	for(i=0; i<7; i++)
	{	for(j=0; j<7; j++)
		{	if(active & mask[7*i+j])
			{	if(selected==7*i+j)
					dc.SetPen(wxPen(wxColor(255,175,175), 2));
				if((~lastactive) & mask[7*i+j])
					dc.SetPen(wxPen(wxColor(255, 0, 0), 2));
					//dc.SetPen(*wxRED_PEN);
				if(board & mask[7*i+j])
					dc.SetBrush(*wxBLUE_BRUSH);
				else
					dc.SetBrush(*wxGREEN_BRUSH);
				dc.DrawCircle(wxPoint(32*i+16,32*j+16), 14);
				dc.SetPen(*wxBLACK_PEN);
			}
			else
			{	
				if((lastactive & mask[7*i+j]) && (~active & mask[7*i+j]))
					dc.SetPen(wxPen(wxColor(255, 0, 0), 2));
					//dc.SetPen(*wxRED_PEN);
				else
					dc.SetPen(*wxWHITE_PEN);

				dc.SetBrush(*wxWHITE_BRUSH);
				dc.DrawCircle(wxPoint(32*i+16,32*j+16), 14);
				dc.SetPen(*wxBLACK_PEN);
			}
		}
	}

	if(player=='O')
		board = ~board;

	char str[21];
	if(score(board&active)-score(~board&active)==0)
		sprintf(str, "Score: 0");
	else if((player=='X' ? 1 : -1)*(score(board&active)-score(~board&active))>0)
		sprintf(str, "Score: B+%i", (player=='X' ? 1 : -1)*(score(board&active)-score(~board&active)));
	else
		sprintf(str, "Score: G+%i", (player=='X' ? 1 : -1)*(score(~board&active)-score(board&active)));
	wxString wxstr = wxString::FromAscii(str);

	statusbar->SetStatusText(wxstr, 0);

	if(active==FULLBOARD || (board&active)==0 || ((~board)&active)==0)
	{	sprintf(str, "%s wins", (player=='X' ? 1 : -1)*(score(board&active)-score(~board&active)) > 0 ? "Blue" : "Green");
		wxstr = wxString::FromAscii(str);
		statusbar->SetStatusText(wxstr, 1);
	}
	else
	{	sprintf(str, "Turn: %s", player=='X' ? "Blue" : "Green");
		wxstr = wxString::FromAscii(str);
		statusbar->SetStatusText(wxstr, 1);
	}
}

void MainPanel::Paint(wxPaintEvent& event)
{	wxPaintDC dc(this);

	DrawBoard(dc);
}
 
class MyApp : public wxApp
{	bool OnInit();
	wxFrame* frame;
	//MainPanel* drawPane;
};

void MainFrame::Reset(wxCommandEvent& WXUNUSED(event))
{	count = 0;
	panel->selected = -1;
	panel->board = STARTBOARD;
	panel->active = STARTACTIVE;
	panel->lastactive = STARTACTIVE;
	panel->player = 'X';
	wxClientDC dc(panel);
	panel->DrawBoard(dc);
}

void doMove(wxString move);

void MainFrame::Load(wxCommandEvent& WXUNUSED(event))
{	
	wxFileDialog* OpenDialog = new wxFileDialog(this, _T("Choose a game to open"), wxEmptyString, wxEmptyString, _("Text files (*.txt)|*.txt"), wxFD_OPEN, wxDefaultPosition);
 
	if(OpenDialog->ShowModal()!=wxID_OK)
	{	OpenDialog->Destroy();
		return;
	}
	
	panel->selected = -1;
	panel->board = STARTBOARD;
	panel->active = STARTACTIVE;
	panel->lastactive = STARTACTIVE;
	panel->player = 'X';
	count=0;
	
	long itemIndex = -1;
	while(true)
	{	itemIndex = MoveList->GetNextItem(itemIndex);
		if(itemIndex==-1)
			break;
		else
		{	MoveList->DeleteItem(itemIndex);
			itemIndex = -1;
		}
	}
	
 	wxTextFile file(OpenDialog->GetPath());
	file.Open();
	OpenDialog->Destroy();
	
	for(unsigned int i=0; i<file.GetLineCount(); i++)
	{	wxString wxstr = file.GetLine(i).Trim();
		if(wxstr.Find(_T("\t"))==wxNOT_FOUND)
		{	doMove(wxstr);
			MoveList->InsertItem(count/2, wxstr);
			count++;
			maxcount=count;
		}
		else
		{
			wxString move1 = wxstr.Mid(0, wxstr.Find(_T("\t")));
			wxString move2 = wxstr.Mid(1+wxstr.Find(_T("\t")));
			
			doMove(move1);
			MoveList->InsertItem(count/2, move1);
			count++;
			maxcount=count;
		
			doMove(move2);
			MoveList->SetItem(count/2, 1, move2);
			count++;
			maxcount=count;			
		
		}
	}
	
	wxClientDC dc(panel);
	panel->DrawBoard(dc);
}

void doMove(wxString move)
{	if(move.Cmp(_T("pass"))!=0)
	{	int x, y, u, v;
		if(move.Len()==2)
		{	y = move.GetChar(1)-'1';
			x = move.GetChar(0)-'a';
		}
		else
		{	y = move.GetChar(3)-'1';
			x = move.GetChar(2)-'a';
			v = move.GetChar(1)-'1';
			u = move.GetChar(0)-'a';
		}
		panel->board |= mask[7*x+y] | clonemask[7*x+y];
		panel->lastactive = panel->active;
		panel->active |= mask[7*x+y];
		if(move.Len()==4)
			panel->active &= ~mask[7*u+v];
	}
	panel->board = ~panel->board;
	panel->player = (panel->player=='X' ? 'O' : 'X');
}

void MainFrame::Undo()
{	if(count==0)
		return;
	count--;

	panel->selected = -1;
	panel->board = STARTBOARD;
	panel->active = STARTACTIVE;
	panel->lastactive = STARTACTIVE;
	panel->player = 'X';

	for(int i=0; i<count; i++)
	{
		wxListItem row_info;
		row_info.SetId(i/2);
		row_info.SetColumn(i%2);
		MoveList->GetItem(row_info);
		doMove(row_info.GetText());
	}

	wxClientDC dc(panel);
	panel->DrawBoard(dc);
}

void MainFrame::Redo()
{	if(count==maxcount)
		return;
	count++;

	panel->selected = -1;
	panel->board = STARTBOARD;
	panel->active = STARTACTIVE;
	panel->lastactive = STARTACTIVE;
	panel->player = 'X';

	for(int i=0; i<count; i++)
	{
		wxListItem row_info;
		row_info.SetId(i/2);
		row_info.SetColumn(i%2);
		MoveList->GetItem(row_info);
		doMove(row_info.GetText());
	}

	wxClientDC dc(panel);
	panel->DrawBoard(dc);
}

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(NULL, -1, title, pos, size, style)
{	panel = new MainPanel(this);

	wxMenuBar* menubar = new wxMenuBar;
	gamemenu = new wxMenu;
	gamemenu->Append(ID_NEW_GAME, wxT("&New game"));
	gamemenu->Append(ID_AI_MOVE, wxT("&AI move"));
	gamemenu->Append(ID_EDIT_MODE, wxT("&Edit mode"), "", wxITEM_CHECK);
	gamemenu->Append(ID_LOAD_GAME, wxT("&Load game..."));
	gamemenu->Append(ID_SAVE_GAME, wxT("&Save game..."));
	bluemenu = new wxMenu;
	bluemenu->Append(ID_BLUE_OFF, wxT("&Off"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_1, wxT("&1"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_2, wxT("&2"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_3, wxT("&3"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_4, wxT("&4"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_5, wxT("&5"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_6, wxT("&6"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_7, wxT("&7"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_8, wxT("&8"), "", wxITEM_RADIO);
	bluemenu->Append(ID_BLUE_9, wxT("&9"), "", wxITEM_RADIO);
	greenmenu = new wxMenu;
	greenmenu->Append(ID_GREEN_OFF, wxT("&Off"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_1, wxT("&1"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_2, wxT("&2"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_3, wxT("&3"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_4, wxT("&4"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_5, wxT("&5"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_6, wxT("&6"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_7, wxT("&7"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_8, wxT("&8"), "", wxITEM_RADIO);
	greenmenu->Append(ID_GREEN_9, wxT("&9"), "", wxITEM_RADIO);
	greenmenu->Check(ID_GREEN_5, 1);

	Connect(ID_NEW_GAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Reset));
	Connect(ID_AI_MOVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::RunAI));
	Connect(ID_EDIT_MODE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::SetEdit));
	Connect(ID_SAVE_GAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Write));
	Connect(ID_LOAD_GAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::Load));

	menubar->Append(gamemenu, wxT("&Game"));
	menubar->Append(bluemenu, wxT("&Blue AI"));
	menubar->Append(greenmenu, wxT("G&reen AI"));

	SetMenuBar(menubar);

	MoveList = new wxListCtrl(this, wxID_ANY, wxPoint(230, 0), wxSize(-1,-1), wxLC_REPORT);
      
	wxListItem col0;
	col0.SetId(0);
	col0.SetText( _T("Blue") );
	col0.SetWidth(100);
	MoveList->InsertColumn(0, col0);
	
	wxListItem col1;
	col1.SetId(1);
	col1.SetText( _T("Green") );
	col1.SetWidth(100);
	MoveList->InsertColumn(1, col1);

	int styles[2] = {wxSB_SUNKEN, wxSB_SUNKEN};
	statusbar = CreateStatusBar(2, wxSB_SUNKEN);
	SetStatusBarPane(-1);
	statusbar->SetStatusText(wxT("Score: 0"), 0);
	statusbar->SetStatusText(wxT("Turn: Blue"), 1);
	statusbar->SetStatusStyles(2, styles);

	SetClientSize(224, 224);
	panel->SetFocus();
}

void MainFrame::RunAI(wxCommandEvent& event)
{	
	if(panel->active==FULLBOARD || (panel->board&panel->active)==0 || ((~panel->board)&panel->active)==0)
		return;

	long depth1 = 5, depth2 = 5;
	for(int i=1; i<10; i++)
	{	if(bluemenu->IsChecked(ID_BLUE_OFF+i))
			depth1 = i;
		if(greenmenu->IsChecked(ID_GREEN_OFF+i))
			depth2 = i;
	}

	panel->lastactive = panel->active;
	negamax(panel->board, panel->active, panel->player=='X' ? depth1 : depth2, -50, 50, &(panel->board), &(panel->active));
	panel->board = ~panel->board;
	panel->player = (panel->player=='X' ? 'O' : 'X');
	panel->selected = -1;

	char* str = (char*)calloc(5, sizeof(char));
	strmove(panel->active, panel->lastactive, str);
	if(count%2==0)
	{	

		long itemIndex = -1;
	 	while(true)
		{	itemIndex = MoveList->GetNextItem(itemIndex);
	 		if(itemIndex==-1)
				break;
	 		if(itemIndex>=count/2)
			{	MoveList->DeleteItem(itemIndex);
				itemIndex = -1;
			}
		}

		MoveList->InsertItem(count/2, wxString::FromAscii(str));
	}
	else
	{	

		long itemIndex = -1;
	 	while(true)
		{	itemIndex = MoveList->GetNextItem(itemIndex);
	 		if(itemIndex==-1)
				break;
	 		if(itemIndex>count/2)
			{	MoveList->DeleteItem(itemIndex);
				itemIndex = -1;
			}
		}

		MoveList->SetItem(count/2, 1, wxString::FromAscii(str));
	}
	count++;
	maxcount = count;
	free(str);

	if(panel->player=='X' && !bluemenu->IsChecked(ID_BLUE_OFF))
		RunAI(event);
	if(panel->player=='O' && !greenmenu->IsChecked(ID_GREEN_OFF))
		RunAI(event);

	wxClientDC dc(panel);
	panel->DrawBoard(dc);
}

void MainFrame::SetEdit(wxCommandEvent& WXUNUSED(event))
{	panel->start = (panel->start==0 ? 1 : 0);
	gamemenu->Check(ID_EDIT_MODE, panel->start==0);
}

void MainFrame::Write(wxCommandEvent& WXUNUSED(event))
{	wxListItem row_info;
	wxString cell_contents_string1;
	wxString cell_contents_string2;

	wxDateTime now = wxDateTime::Now();

	int j = 1;
	while(true)
	{	wxTextFile file(now.Format(_T("game%y%m%d-")) + wxString::Format(_T("%d"), j) + _T(".txt"));
		if(!file.Exists())
			break;
		j++;
	}

	wxFileDialog* SaveDialog = new wxFileDialog(this, _T("Choose where to save game"), wxEmptyString, now.Format(_T("game%y%m%d-")) + wxString::Format(_T("%d"), j) + _T(".txt"), _("Text files (*.txt)|*.txt"), wxFD_SAVE, wxDefaultPosition);
 
	if(SaveDialog->ShowModal()!=wxID_OK)
	{	SaveDialog->Destroy();
		return;
	}

	wxTextFile file(SaveDialog->GetPath());

	for(int i=0; i<count; i++)
	{	
		row_info.SetId(i/2);
		row_info.SetColumn(i%2);
		MoveList->GetItem(row_info);
		if(i%2==0)
		{	cell_contents_string1 = row_info.GetText();
			if(i==count-1)
				file.AddLine(cell_contents_string1);
		}
		else
		{	cell_contents_string2 = row_info.GetText();
			file.AddLine(cell_contents_string1+_T("\t")+cell_contents_string2);
		}
	}

	file.Write();
	file.Close();
}

bool MyApp::OnInit()
{
	frame = new MainFrame(wxT("Ataxx AI"), wxPoint(-1, -1), wxSize(224, 224), wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX);
	
	frame->Show();
	SetTopWindow(frame);
	
	return true;
} 

IMPLEMENT_APP(MyApp)
