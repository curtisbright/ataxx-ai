// Ataxx AI by Curtis Bright
// https://github.com/curtisbright/ataxx-ai/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "masks.h"
#define max(x,y) (x > y ? x : y)

// Search tree depth
#define DEPTH 5

#ifdef PRINT
	FILE* fp;
	int count;

// Write the gameboard inside a Graphviz DOT file.
void writeboard(longint cells, longint active)
{	int i;
	for(i=0; i<49; i++)
	{	if(active & mask[i])
		{	if(cells & mask[i])
				fprintf(fp, "X");
			else
				fprintf(fp, "O");
		}
		else
			fprintf(fp, ".");
		fprintf(fp, "\\n");
	}
}
#endif

// Print the gameboard on the screen
void printboard(longint cells, longint active, char player)
{	int i, j;
	if(player=='O')
		cells = ~cells;

	printf("  abcdefg\n");
	for(i=0; i<7; i++)
	{	printf("%i ", i+1);
		for(j=0; j<7; j++)
		{	if(active & mask[7*i+j])
			{	if(cells & mask[7*i+j])
					printf("X");
				else
					printf("O");
			}
			else
				printf(".");
		}
		printf(" %i\n", i+1);
	}
	printf("  abcdefg\n");
}

// Count the number of legal moves
int countmoves(longint cells, longint active)
{	int i, j;
	int nummoves = 0;

	cells &= active;
	longint clonemoves = 0;
	for(i=0; i<49; i++)
		if(cells & mask[i])
			clonemoves |= clonemask[i];

	clonemoves &= ~active;
	for(i=0; i<49; i++)
		if(clonemoves & mask[i])
			nummoves++;

	longint jumpmoves;
	for(i=0; i<49; i++)
		if(cells & mask[i])
		{	jumpmoves = jumpmask[i] & ~active;
			for(j=0; j<49; j++)
				if(jumpmoves & mask[j])
					nummoves++;
		}

	return nummoves;
}

// Check if a move is valid or not
int validmove(char* move, longint cells, longint active)
{	cells &= active;
	if(move[0]>='a' && move[0]<='g' && move[1]>='1' && move[1]<='7' && move[2]=='\n')
	{	if((~active & mask[7*(move[1]-'1')+(move[0]-'a')]) && (cells & clonemask[7*(move[1]-'1')+(move[0]-'a')]))
			return 1;
	}
	else if(move[0]>='a' && move[0]<='g' && move[1]>='1' && move[1]<='7' && move[2]>='a' && move[2]<='g' && move[3]>='1' && move[3]<='7' && move[4]=='\n')
	{	if((~active & mask[7*(move[3]-'1')+(move[2]-'a')]) && (cells & mask[7*(move[1]-'1')+(move[0]-'a')]))
			return 1;
	}
	return 0;
}

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

	board &= active;

#ifdef PRINT
	int myid = 0;
	int tempcount;
	int maxcount=0;
	myid = count;
	count++;
#endif

	// Leaf node
	if(depth==0)
	{
#ifdef PRINT
		fprintf(fp, "%i [fontname=courier, label=\"", myid);
		writeboard(depth%2==(DEPTH+1)%2 ? board : ~board, active);
		depth%2==(DEPTH+1)%2 ? fprintf(fp, "Player: X\\n") : fprintf(fp, "Player: O\\n");
		fprintf(fp, "Score: %i\\n", score(board)-score((~board)&active));
		fprintf(fp, "Depth: %i\"];\n", depth);
#endif

		return score(board)-score((~board)&active);
	}

	// Find all possible clone moves
	longint clonemoves = 0;
	longint tempboard;
	for(tempboard=board; tempboard; tempboard&=tempboard-1)
		clonemoves |= clonemask[ilog2(tempboard&-tempboard)];

	for(clonemoves&=~active; clonemoves; clonemoves&=clonemoves-1)
	{	boards = realloc(boards, (num+1)*sizeof(longint));
		actives = realloc(actives, (num+1)*sizeof(longint));

		boards[num] = board | (clonemoves&-clonemoves) | clonemask[ilog2(clonemoves&-clonemoves)];
		actives[num] = active | (clonemoves&-clonemoves);

		num++;
	}

	// Find all possible jump moves
	longint jumpmoves;
	for(tempboard=board; tempboard; tempboard&=tempboard-1)
	{	jumpmoves = jumpmask[ilog2(tempboard&-tempboard)];
		for(jumpmoves&=~active; jumpmoves; jumpmoves&=jumpmoves-1)
		{	boards = realloc(boards, (num+1)*sizeof(longint));
			actives = realloc(actives, (num+1)*sizeof(longint));

			boards[num] = board | (jumpmoves&-jumpmoves) | clonemask[ilog2(jumpmoves&-jumpmoves)];
			actives[num] = (active | (jumpmoves&-jumpmoves)) & ~(tempboard&-tempboard);

			num++;
		}
	}

	// If no moves possible, pass
	if(num==0)
	{	boards = realloc(boards, sizeof(longint));
		actives = realloc(actives, sizeof(longint));

		boards[0] = board;
		actives[0] = active;
		num++;
	}

	// Run search through all nodes
	for(i=0; i<num; i++)
	{	
#ifdef PRINT
		tempcount = count;
		fprintf(fp, "%i -> %i;\n", myid, count);
#endif
		tempscore = -negamax(~boards[i], actives[i], depth-1, -beta, -alpha, optimalboard, optimalactive);

		if(tempscore > maxscore)
		{	maxscore = tempscore;
			alpha = max(alpha, tempscore);
			maxi = i;
#ifdef PRINT
			maxcount = tempcount;
#endif
		}
		if(alpha>=beta)
			break;
	}

	*optimalboard = boards[maxi];
	*optimalactive = actives[maxi];
	free(boards);
	free(actives);

#ifdef PRINT
	fprintf(fp, "%i [fontname=courier, label=\"", myid);
	writeboard(depth%2==(DEPTH+1)%2 ? board : ~board, active);
	depth%2==(DEPTH+1)%2 ? fprintf(fp, "Player: X\\n") : fprintf(fp, "Player: O\\n");
	fprintf(fp, "Score: %i\\n", score(board)-score((~board)&active));
	fprintf(fp, "Alpha: %i\\n", alpha);
	fprintf(fp, "Beta: %i\\n", beta);
	fprintf(fp, "Depth: %i\"];\n", depth);
	fprintf(fp, "%i [color=red];\n", maxcount);
#endif

	return alpha;
}

// Main entry procedure
int main(int argc, char** argv)
{
	longint cells  = STARTBOARD;
	longint active = STARTACTIVE;

	char player = 'X';
	char move[256];
	int nummoves;

#ifdef PRINT
	int movenum = 0;
	char filename[12];
#endif

	printf("Ataxx AI by Curtis Bright\n");

	// Check if game has ended
	while(!(active==FULLBOARD || (cells&active)==0 || ((~cells)&active)==0))
	{	nummoves = countmoves(cells, active);
		if(nummoves==0)
			printf("No moves for %c.  Passing.\n", player);
		else
		{	
			// Print user interface
			printboard(cells, active, player);
			if(player=='X')
				printf("X Score: %i\n", score(cells&active)-score((~cells)&active));
			else
				printf("X Score: %i\n", -score(cells&active)+score((~cells)&active));

			move[0] = '\0';

			// Get move
			if(player=='X')
			{	while(!validmove(move, cells, active))
				{	printf("Move choice for %c: ", player);
					if(fgets(move, 256, stdin)==NULL || move[0]=='q')
						return 0;
				}
			}
			else
			{	
#ifdef PRINT
				movenum++;
				count = 0;
				sprintf(filename, "move%i.dot", movenum);
				fp = fopen(filename, "w");
				fprintf(fp, "digraph G {\n");
#endif
				// The AI negamax search
				negamax(cells, active, DEPTH, -50, 50, &cells, &active);

#ifdef PRINT
				fprintf(fp, "0 [color=red]\n}\n");
				fclose(fp);
#endif
			}

			// Update board state after move
			if(move[2]=='\n' && move[0]!='\0')
			{	cells |= mask[7*(move[1]-'1')+(move[0]-'a')] | clonemask[7*(move[1]-'1')+(move[0]-'a')];
				active |= mask[7*(move[1]-'1')+(move[0]-'a')];
			}
			else if(move[4]=='\n' && move[0]!='\0')
			{	cells |= mask[7*(move[3]-'1')+(move[2]-'a')] | clonemask[7*(move[3]-'1')+(move[2]-'a')];
				active |= mask[7*(move[3]-'1')+(move[2]-'a')];
				if(!(mask[7*(move[3]-'1')+(move[2]-'a')] & clonemask[7*(move[1]-'1')+(move[0]-'a')]))
					active &= ~mask[7*(move[1]-'1')+(move[0]-'a')];
			}
		}
		// Switch to next player
		cells = ~cells;
		player = (player=='X' ? 'O' : 'X');
	}

	// Print final game state
	printboard(cells, active, player);
	if(player=='X')
		printf("X Score: %i\n", score(cells&active)-score((~cells)&active));
	else
		printf("X Score: %i\n", -score(cells&active)+score((~cells)&active));

	return 0;
}
