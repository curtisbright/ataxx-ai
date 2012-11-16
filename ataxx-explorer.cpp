#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/textfile.h"
#include "masks.h"
#include <stdio.h>
#define max(x,y) (x > y ? x : y)

wxTextCtrl* Depth1Text;
wxTextCtrl* Depth2Text;
wxStaticText* ScoreText;
wxStaticText* TurnText;
wxButton* AIButton;
wxButton* EditButton;
wxButton* WriteButton;
wxButton* ResetButton;
wxButton* UndoButton;
wxButton* RedoButton;
wxButton* LoadButton;
wxButton* CheckAIButton;
wxListCtrl* MoveList;
int count = 0;
int maxcount = 0;

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

class MainPanel : public wxPanel
{
//private:
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
	
	int start;
	//wxFrame* par;
	
//public:
	MainPanel(wxFrame* parent);
};

MainPanel* panel;

MainPanel::MainPanel(wxFrame* parent) : wxPanel(parent, wxID_ANY, wxPoint(0, 0), wxSize(224, 224), wxTAB_TRAVERSAL, _T("panel"))
{	selected = -1;
	board = STARTBOARD;
	active = STARTACTIVE;
	lastactive = STARTACTIVE;
	player = 'X';
	
	//par = parent;

	/*negamax(board, active, DEPTH, -50, 50, &board, &active);
	board = ~board;

	player = (player=='X' ? 'O' : 'X');*/
	
	start = 1;
	
	Connect(wxEVT_PAINT, wxPaintEventHandler(MainPanel::Paint));
	Connect(wxEVT_LEFT_UP, wxMouseEventHandler(MainPanel::LMouseUp));
	Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(MainPanel::RMouseUp));
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

	if(active==FULLBOARD)
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

	char str[12];
	sprintf(str, "Score: %i", player=='X' ? score(board&active)-score(~board&active) : score(~board&active)-score(board&active));
	wxString wxstr = wxString::FromAscii(str);

	//dc.DrawText(wxstr, 0, 224);
	ScoreText->SetLabel(wxstr);

	sprintf(str, "Turn: %s", player=='X' ? "Blue" : "Green");
	wxstr = wxString::FromAscii(str);
	TurnText->SetLabel(wxstr);
}

void MainPanel::Paint(wxPaintEvent& event)
{	wxPaintDC dc(this);

    /*dc.SetPen(*wxBLACK_PEN);

	int i, j;
	for(i=0; i<7; i++)
		for(j=0; j<7; j++)
			dc.DrawRectangle(32*i, 32*j, 32, 32);
	*/

	DrawBoard(dc);
}
 
class MyApp : public wxApp
{	bool OnInit();
	wxFrame* frame;
	//MainPanel* drawPane;
};

class MainFrame : public wxFrame
{	
private:
	
	void Reset(wxCommandEvent& event);
	void RunAI(wxCommandEvent& event);
	void SetEdit(wxCommandEvent& event);
	void Write(wxCommandEvent& event);
	void Resize(wxSizeEvent& event);
	void Undo(wxCommandEvent& event);
	void Redo(wxCommandEvent& event);
	void Load(wxCommandEvent& event);
	void CheckAI(wxCommandEvent& event);

public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
};

void MainFrame::Resize(wxSizeEvent& WXUNUSED(event))
{	if(this->GetClientSize().GetHeight()>30)
		MoveList->SetClientSize(200, this->GetClientSize().GetHeight());
}

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
	wxFileDialog* OpenDialog = new wxFileDialog(this, _T("Choose a file to open"), wxEmptyString, wxEmptyString, _("Text files (*.txt)|*.txt"), wxFD_OPEN, wxDefaultPosition);
 
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
	
	//wxMessageBox(file.GetLine(0), _T("About Hello World"), wxOK | wxICON_INFORMATION, this);
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

void MainFrame::Undo(wxCommandEvent& WXUNUSED(event))
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

void MainFrame::Redo(wxCommandEvent& WXUNUSED(event))
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

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame(NULL, -1, title, pos, size)
{	panel = new MainPanel(this);

	AIButton = new wxButton(this, wxID_HIGHEST+1, _T("AI Move"), wxPoint(100, 225));
	Connect(wxID_HIGHEST+1, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::RunAI));
	EditButton = new wxButton(this, wxID_HIGHEST+2, _T("Edit On"), wxPoint(100, 250));
	Connect(wxID_HIGHEST+2, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::SetEdit));
	WriteButton = new wxButton(this, wxID_HIGHEST+3, _T("Write"), wxPoint(100, 275));
	Connect(wxID_HIGHEST+3, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::Write));
	ResetButton = new wxButton(this, wxID_HIGHEST+4, _T("Reset"), wxPoint(100, 300));
	Connect(wxID_HIGHEST+4, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::Reset));
	UndoButton = new wxButton(this, wxID_HIGHEST+5, _T("Undo"), wxPoint(100, 325));
	Connect(wxID_HIGHEST+5, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::Undo));
	RedoButton = new wxButton(this, wxID_HIGHEST+6, _T("Redo"), wxPoint(100, 350));
	Connect(wxID_HIGHEST+6, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::Redo));
	LoadButton = new wxButton(this, wxID_HIGHEST+7, _T("Load"), wxPoint(100, 375));
	Connect(wxID_HIGHEST+7, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::Load));
	CheckAIButton = new wxButton(this, wxID_HIGHEST+8, _T("Check AI"), wxPoint(100, 400));
	Connect(wxID_HIGHEST+8, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::CheckAI));
	ScoreText = new wxStaticText(this, wxID_ANY, _T("Score: 0"), wxPoint(0, 225));
	TurnText = new wxStaticText(this, wxID_ANY, _T("Turn: Blue"), wxPoint(0, 250));
	Depth1Text = new wxTextCtrl(this, wxID_ANY, _T("5"), wxPoint(0, 275));
	Depth2Text = new wxTextCtrl(this, wxID_ANY, _T("5"), wxPoint(0, 300));

	MoveList = new wxListCtrl(this, wxID_ANY, wxPoint(230, 0), wxSize(-1,-1), wxLC_REPORT);

	Connect(wxEVT_SIZE, wxSizeEventHandler(MainFrame::Resize));
      
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

	/*MoveList->InsertItem(0, "1");
	MoveList->InsertItem(1, "2");
	MoveList->SetItem(0, 1, "17:00");
	MoveList->SetItem(1, 1, "18:00");*/

	/*wxTextFile file;
	file.Create("test.txt");
	file.AddLine("Test\n");
	file.Write();
	file.Close();*/
}

void MainFrame::RunAI(wxCommandEvent& WXUNUSED(event))
{	//wxMessageBox(_("This is a wxWidgets Hello world sample"), _("About Hello World"), wxOK | wxICON_INFORMATION, this);
	
	if(panel->active==FULLBOARD)
		return;

	long depth1, depth2;
	Depth1Text->GetValue().ToLong(&depth1);
	Depth2Text->GetValue().ToLong(&depth2);

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
	
	wxClientDC dc(panel);
	panel->DrawBoard(dc);
}

void MainFrame::CheckAI(wxCommandEvent& WXUNUSED(event))
{		
	if(panel->active==FULLBOARD)
		return;

	long depth1, depth2;
	Depth1Text->GetValue().ToLong(&depth1);
	Depth2Text->GetValue().ToLong(&depth2);

	panel->lastactive = panel->active;
	negamax(panel->board, panel->active, panel->player=='X' ? depth1 : depth2, -50, 50, &(panel->board), &(panel->active));
	panel->board = ~panel->board;
	panel->player = (panel->player=='X' ? 'O' : 'X');
	panel->selected = -1;
	
	wxClientDC dc(panel);
	panel->DrawBoard(dc);
}

void MainFrame::SetEdit(wxCommandEvent& WXUNUSED(event))
{	panel->start = (panel->start==0 ? 1 : 0);
	EditButton->SetLabel(panel->start==0 ? _T("Edit Off") : _T("Edit On"));
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

	wxTextFile file(now.Format(_T("game%y%m%d-")) + wxString::Format(_T("%d"), j) + _T(".txt"));

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
	frame = new MainFrame(wxT("Ataxx Explorer"), wxPoint(-1, -1), wxSize(500, 400));
	
	frame->Show();
	SetTopWindow(frame);
	
	return true;
} 

IMPLEMENT_APP(MyApp)
