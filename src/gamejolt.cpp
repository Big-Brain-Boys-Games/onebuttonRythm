#ifdef GAMEJOLT
#include "../deps/game-jolt-api-cpp-library/source/gjAPI.h"

#include <stdio.h>
#include <stdlib.h>
#include "apikey.h"

#include <string>

gjAPI _api;
#endif

#include "gamejolt.h"
extern char _playerName[100];

void apiInit()
{
    #ifdef GAMEJOLT
    _api.Init(716890, apikey);
    #endif
}

void apiUpdate()
{
    #ifdef GAMEJOLT
    _api.Update();
    #endif
}


void submitScore(int mapID, int points, int * rank)
{
    #ifdef GAMEJOLT
    // direct access a score table object and add guest score (may block if score table is not cached)
    char str [100];
    sprintf(str, "%i", points);
    _api.InterScore()->GetScoreTable(mapID)->AddGuestScoreCall(str, points, "", _playerName);
 
    // fetch all score tables now (may block if score tables are not cached)
    gjScoreTableMap apScoreTableMap;
    if(_api.InterScore()->FetchScoreTablesNow(&apScoreTableMap) == GJ_OK)
    {
        for(auto it = apScoreTableMap.begin(); it != apScoreTableMap.end(); ++it)
        {
            gjScoreTable* pScoreTable = it->second;
 
            // and fetch the first 10 score entries of each score table now (blocks)
            gjScoreList apScoreList;
            if(pScoreTable->FetchScoresNow(false, 500, &apScoreList) == GJ_OK)
            {
                // and fetch the associated users with a callback (does not block)
                for(size_t i = 0; i < apScoreList.size(); ++i)
                {
                    gjScore* pScore = apScoreList[i];
                    std::string score = pScore->GetScore();
                    int otherPoints = stoi(score);
                    if(otherPoints < points)
                    {
                        *rank = i;
                        break;
                    }
                }
            }
        }
    }
    #endif
}