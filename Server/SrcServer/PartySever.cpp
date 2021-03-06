#define WINVER 0x0501
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <io.h>
#include <stdio.h>         /* for sprintf_s                           */
#include <string.h>        /* for strlen                            */
#include <memory.h>
#include <process.h>       /* for _beginthread                      */
//#include <winsock2.h>
//#include <mswsock.h>

#include "..\\resource.h"
#include "..\\smwsock.h"

#include "..\\smlib3d\\smd3d.h"
#include "..\\character.h"
#include "..\\fileread.h"
#include "..\\playmain.h"
#include "..\\field.h"
#include "..\\sinbaram\\sinlinkheader.h"
#include "..\\language.h"
#include "partymsg.h"
#include "onserver.h"

extern DWORD	dwPlayServTime;

/*
	//파티장
	int	CreateParty( rsPLAYINFO *lpPlayInfo );			//파티생성 - ( 초기 파티원 정보 )
	int	JoinPartyPlayer( rsPLAYINFO *lpPlayInfo );		//파티에 가입
	int	DeletePartyPlayer( rsPLAYINFO *lpPlayInfo );	//파티에서 탈퇴
	int	UpdatePartyPlayer();							//파티원 업데이트
	int	LeavePartyMaster( rsPLAYINFO *lpPlayInfo );		//파티장 이임
	int	ReleaseParty();									//파티장 해산

	//파티원
	int	JoinParty( rsPLAYINFO *lpPlayMaster );			//파티 가입 신청
	int	SecedeParty ( rsPLAYINFO *lpPlayMaster );		//파티 탈퇴
*/
/*
	////////////// 파티 관련 정보 ///////////////////
	DWORD			dwPartyInfo;						//파티 정보
	rsPLAYINFO		*lpPartyMaster;						//파티장 포인트
	DWORD			PartyMasterObjectCode;				//파티장 오브젝트 코드
	rsPLAYINFO		*lpPartyPlayers[PARTY_PLAYER_MAX];	//파티원 포인트
	DWORD			dwPartyObjectCode[PARTY_PLAYER_MAX];//파티원의 오브젝트 코드
*/

//캐릭터를 찾는다 ( 나중에 빨리 칮을수 있게 코드화 시켜 바꿔 줘야 함 )
rsPLAYINFO *srFindUserFromSerial(DWORD dwObjectSerial);
//아이템 정보송수신 구조체에 보안코드 삽입 ( 컴파일시 서버에서만 존재 )
int	rsRegist_ItemSecCode(rsPLAYINFO *lpPlayInfo, TRANS_ITEMINFO *lpTransItemInfo, int NewItem);


//파티원에게 데이타 보냄
int rsPLAYINFO::SendPartyData(rsPLAYINFO *lpPlayInfo_From, char *szData)
{
	int size;
	int cnt;


	size = ((smTRANS_COMMAND *)szData)->size;

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt] != lpPlayInfo_From)
		{
			if (lpPartyPlayers[cnt]->lpsmSock)
			{
				lpPartyPlayers[cnt]->lpsmSock->Send2(szData, size, TRUE);
			}
		}
	}


	return TRUE;
}

//파티원에게 메세지 보냄
int rsPLAYINFO::SendPartyMessage(char *szMessage, DWORD dwCode)
{
	TRANS_CHATMESSAGE	TransChatMessage;
	int cnt;

	if (lpPartyMaster)
	{
		lstrcpy(TransChatMessage.szMessage, szMessage);
		TransChatMessage.code = smTRANSCODE_WHISPERMESSAGE;
		TransChatMessage.size = 32 + lstrlen(TransChatMessage.szMessage);
		TransChatMessage.dwIP = dwCode;
		TransChatMessage.dwObjectSerial = 0;

		for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
		{
			if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt]->lpsmSock)
			{
				lpPartyPlayers[cnt]->lpsmSock->Send((char *)&TransChatMessage, TransChatMessage.size, TRUE);
			}
		}
	}
	else
	{
		lstrcpy(TransChatMessage.szMessage, szMessage);
		TransChatMessage.code = smTRANSCODE_WHISPERMESSAGE;
		TransChatMessage.size = 32 + lstrlen(TransChatMessage.szMessage);
		TransChatMessage.dwIP = 0;
		TransChatMessage.dwObjectSerial = 0;
		if (lpsmSock)
			lpsmSock->Send((char *)&TransChatMessage, TransChatMessage.size, TRUE);
	}

	return TRUE;
}


//파티생성
int rsPLAYINFO::CreateParty(rsPLAYINFO *lpPlayInfo)
{
	int cnt;

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		lpPartyPlayers[cnt] = 0;
		dwPartyObjectCode[0] = 0;
	}

	PartyUpdateFlag = 0;
	dwPartyInfo = PARTY_STATE_MASTER;
	lpPartyMaster = this;

	//파티원 세팅
	lpPartyPlayers[0] = this;
	dwPartyObjectCode[0] = dwObjectSerial;
	if (this != lpPlayInfo)
	{
		lpPartyPlayers[1] = lpPlayInfo;
		dwPartyObjectCode[1] = lpPlayInfo->dwObjectSerial;

		lpPlayInfo->dwPartyInfo = PARTY_STATE_MEMBER;
		lpPlayInfo->lpPartyMaster = this;
	}

	UpdatePartyPlayer();

	return TRUE;
}

//파티에 가입
int	rsPLAYINFO::JoinPartyPlayer(rsPLAYINFO *lpPlayInfo)
{
	int cnt;

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt] == lpPlayInfo)
		{
			//이미 가입된 유저
			return FALSE;
		}
	}

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (!lpPartyPlayers[cnt])
		{
			lpPartyPlayers[cnt] = lpPlayInfo;
			dwPartyObjectCode[cnt] = lpPlayInfo->dwObjectSerial;
			lpPlayInfo->dwPartyInfo = PARTY_STATE_MEMBER;
			lpPlayInfo->lpPartyMaster = this;
			UpdatePartyPlayer();
			return TRUE;
		}
	}

	return FALSE;
}

//파티에서 탈퇴
int	rsPLAYINFO::DeletePartyPlayer(rsPLAYINFO *lpPlayInfo)
{
	int cnt;
	smTRANS_COMMAND		TransCommand;

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt] == lpPlayInfo)
		{
			lpPartyPlayers[cnt]->dwPartyInfo = PARTY_STATE_NONE;
			lpPartyPlayers[cnt]->lpPartyMaster = NULL;
			lpPartyPlayers[cnt] = 0;
			dwPartyObjectCode[cnt] = 0;
			UpdatePartyPlayer();


			if (lpPlayInfo->lpsmSock)
			{
				TransCommand.code = smTRANSCODE_PARTY_RELEASE;
				TransCommand.size = sizeof(smTRANS_COMMAND);
				TransCommand.WParam = dwObjectSerial;
				TransCommand.LParam = 0;
				TransCommand.SParam = 0;

				lpPlayInfo->lpsmSock->Send((char *)&TransCommand, TransCommand.size, TRUE);
			}
			return TRUE;
		}
	}

	return FALSE;
}

//파티원 변경
int	rsPLAYINFO::ChangePartyPlayer(rsPLAYINFO *lpOldPlayInfo, rsPLAYINFO *lpNewPlayInfo)
{
	int cnt;

	if (!lpPartyMaster)
	{
		//파티장이 아닌 경우 ( 파티원 해제 )
		lpNewPlayInfo->dwPartyInfo = PARTY_STATE_NONE;
		lpNewPlayInfo->lpPartyMaster = NULL;
		return FALSE;
	}

	//변경된 파티원이 파티장일 경우
	if (lpOldPlayInfo == lpPartyMaster)
	{
		for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
		{
			if (lpPartyPlayers[cnt])
			{
				lpPartyPlayers[cnt]->lpPartyMaster = lpNewPlayInfo;
			}
		}
		lpNewPlayInfo->dwPartyInfo = PARTY_STATE_MASTER;
		lpNewPlayInfo->lpPartyMaster = lpNewPlayInfo;
	}

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyMaster->lpPartyPlayers[cnt] == lpOldPlayInfo)
		{
			lpPartyMaster->lpPartyPlayers[cnt] = lpNewPlayInfo;
			break;
		}
	}

	lpOldPlayInfo->dwPartyInfo = PARTY_STATE_NONE;
	lpOldPlayInfo->lpPartyMaster = NULL;

	//lpNewPlayInfo->dwPartyInfo = PARTY_STATE_NONE;
	//lpNewPlayInfo->lpPartyMaster = NULL;

	return TRUE;
}

//파티원 정보 업데이트
int	rsPLAYINFO::UpdatePartyPlayer()
{
	int cnt, cnt2;
	TRANS_PARTY_USERS	TransPartyUsers;
	int	LevelAvg;

	cnt = 0;
	cnt2 = 0;

	if (!lpPartyMaster) return FALSE;

	//파티장 정보
	TransPartyUsers.PartyUser[cnt2].dwObjectSerial = lpPartyMaster->dwObjectSerial;
	lstrcpy(TransPartyUsers.PartyUser[cnt2].szName, lpPartyMaster->smCharInfo.szName);
	lstrcpy(TransPartyUsers.PartyUser[cnt2].szModelName, lpPartyMaster->smCharInfo.szModelName);
	lstrcpy(TransPartyUsers.PartyUser[cnt2].szModelName2, lpPartyMaster->smCharInfo.szModelName2);
	TransPartyUsers.PartyUser[cnt2].Life[0] = lpPartyMaster->smCharInfo.Life[0];
	TransPartyUsers.PartyUser[cnt2].Life[1] = lpPartyMaster->smCharInfo.Life[1];
	TransPartyUsers.PartyUser[cnt2].Level = lpPartyMaster->smCharInfo.Level;
	TransPartyUsers.PartyUser[cnt2].x = lpPartyMaster->Position.x;
	TransPartyUsers.PartyUser[cnt2].z = lpPartyMaster->Position.z;
	LevelAvg = lpPartyMaster->smCharInfo.Level;
	cnt2++;

	//파티원 정보
	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt] != lpPartyMaster)
		{
			TransPartyUsers.PartyUser[cnt2].dwObjectSerial = lpPartyPlayers[cnt]->dwObjectSerial;
			lstrcpy(TransPartyUsers.PartyUser[cnt2].szName, lpPartyPlayers[cnt]->smCharInfo.szName);
			lstrcpy(TransPartyUsers.PartyUser[cnt2].szModelName, lpPartyPlayers[cnt]->smCharInfo.szModelName);
			lstrcpy(TransPartyUsers.PartyUser[cnt2].szModelName2, lpPartyPlayers[cnt]->smCharInfo.szModelName2);
			TransPartyUsers.PartyUser[cnt2].Life[0] = lpPartyPlayers[cnt]->smCharInfo.Life[0];
			TransPartyUsers.PartyUser[cnt2].Life[1] = lpPartyPlayers[cnt]->smCharInfo.Life[1];
			TransPartyUsers.PartyUser[cnt2].Level = lpPartyPlayers[cnt]->smCharInfo.Level;
			TransPartyUsers.PartyUser[cnt2].x = lpPartyPlayers[cnt]->Position.x;
			TransPartyUsers.PartyUser[cnt2].z = lpPartyPlayers[cnt]->Position.z;
			LevelAvg += lpPartyPlayers[cnt]->smCharInfo.Level;
			cnt2++;
		}
	}

	PartyUserCount = cnt2;
	PartyLevelAverage = LevelAvg / cnt2;

	ZeroMemory(TransPartyUsers.dwTemp, sizeof(DWORD) * 8);

	TransPartyUsers.dwObjectSerial = dwObjectSerial;
	TransPartyUsers.PartyUserCount = cnt2;
	TransPartyUsers.size = sizeof(TRANS_PARTY_USERS) - ((PARTY_PLAYER_MAX - cnt2)*sizeof(_PARTY_USER));
	TransPartyUsers.code = smTRANSCODE_PARTY_UPDATE;

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt]->lpsmSock)
		{
			lpPartyPlayers[cnt]->lpsmSock->Send((char *)&TransPartyUsers, TransPartyUsers.size, TRUE);
		}
	}

	return TRUE;
}
/*
struct _PARTY_PLAYINFO {
	DWORD	dwObjectSerial;			//객체 고유번호
	DWORD	Level;					//레벨
	short	Life[2];				//생명력
};

struct	TRANS_PARTY_PLAYINFO {
	int		size,code;

	DWORD	dwObjectSerial;
	DWORD	dwTemp;

	int		PartyUserCount;

	_PARTY_PLAYINFO	PlayInfo[PARTY_PLAYER_MAX];
};
*/

//파티원 정보 업데이트
int	rsPLAYINFO::UpdatePartyPlayInfo()
{
	int cnt, cnt2;
	int	LevelAvg;
	TRANS_PARTY_PLAYINFO	TransPartyPlayInfo;

	cnt = 0;
	cnt2 = 0;

	PartyUpdateFlag = 0;

	//파티장 정보
	TransPartyPlayInfo.PlayInfo[cnt2].dwObjectSerial = dwObjectSerial;
	TransPartyPlayInfo.PlayInfo[cnt2].Life[0] = smCharInfo.Life[0];
	TransPartyPlayInfo.PlayInfo[cnt2].Life[1] = smCharInfo.Life[1];
	TransPartyPlayInfo.PlayInfo[cnt2].Level = smCharInfo.Level;
	TransPartyPlayInfo.PlayInfo[cnt2].x = Position.x;
	TransPartyPlayInfo.PlayInfo[cnt2].z = Position.z;
	LevelAvg = smCharInfo.Level;
	cnt2++;

	//파티원 정보
	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt] != this)
		{
			TransPartyPlayInfo.PlayInfo[cnt2].dwObjectSerial = lpPartyPlayers[cnt]->dwObjectSerial;
			TransPartyPlayInfo.PlayInfo[cnt2].Life[0] = lpPartyPlayers[cnt]->smCharInfo.Life[0];
			TransPartyPlayInfo.PlayInfo[cnt2].Life[1] = lpPartyPlayers[cnt]->smCharInfo.Life[1];
			TransPartyPlayInfo.PlayInfo[cnt2].Level = lpPartyPlayers[cnt]->smCharInfo.Level;
			TransPartyPlayInfo.PlayInfo[cnt2].x = lpPartyPlayers[cnt]->Position.x;
			TransPartyPlayInfo.PlayInfo[cnt2].z = lpPartyPlayers[cnt]->Position.z;
			LevelAvg += lpPartyPlayers[cnt]->smCharInfo.Level;
			cnt2++;
		}
	}

	PartyUserCount = cnt2;
	PartyLevelAverage = LevelAvg / cnt2;

	TransPartyPlayInfo.dwTemp = 0;

	TransPartyPlayInfo.dwObjectSerial = dwObjectSerial;
	TransPartyPlayInfo.PartyUserCount = cnt2;
	TransPartyPlayInfo.size = sizeof(TRANS_PARTY_PLAYINFO);
	TransPartyPlayInfo.code = smTRANSCODE_PARTY_PLAYUPDATE;

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt]->lpsmSock)
		{
			lpPartyPlayers[cnt]->lpsmSock->Send((char *)&TransPartyPlayInfo, TransPartyPlayInfo.size, TRUE);
		}
	}

	return TRUE;
}



//메인 서버에 파티원 정보 업데이트 /smTRANSCODE_PARTY_PLAYUPDATE
int	rsPLAYINFO::UpdateServerPartyPlayer(TRANS_PARTY_PLAYINFO *lpTransPartyPlayInfo)
{
	int cnt, cnt2;
	int LevelAvg;
	rsPLAYINFO *lpPlayInfo;
	DWORD	dwPartyCode1, dwPartyCode2;

	LevelAvg = 0;

	if (dwPartyInfo == PARTY_STATE_MASTER)
	{

		dwPartyCode1 = 0;
		dwPartyCode2 = 0;

		for (cnt = 0; cnt < lpTransPartyPlayInfo->PartyUserCount; cnt++)
		{
			dwPartyCode1 += lpTransPartyPlayInfo->PlayInfo[cnt].dwObjectSerial;
		}

		cnt = 0;
		for (cnt2 = 0; cnt2 < PARTY_PLAYER_MAX; cnt2++)
		{
			if (lpPartyPlayers[cnt2])
			{
				dwPartyCode2 += lpPartyPlayers[cnt2]->dwObjectSerial;
				cnt++;
			}
		}

		if (cnt == lpTransPartyPlayInfo->PartyUserCount && dwPartyCode1 == dwPartyCode2)
		{
			//파티정보가 변한게 없음
			return TRUE;

		}
	}

	//////////// 정보 업데이트 ///////////////
	//파티원 정보 제거
	if (dwPartyInfo == PARTY_STATE_MASTER)
	{
		for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
		{
			if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt]->lpsmSock)
			{
				lpPartyPlayers[cnt]->dwPartyInfo = PARTY_STATE_NONE;
				lpPartyPlayers[cnt]->lpPartyMaster = NULL;
			}
		}
	}

	ZeroMemory(lpPartyPlayers, sizeof(rsPLAYINFO *)*PARTY_PLAYER_MAX);

	for (cnt = 0; cnt < lpTransPartyPlayInfo->PartyUserCount; cnt++)
	{
		if (lpTransPartyPlayInfo->PlayInfo[cnt].dwObjectSerial == dwObjectSerial)
		{
			dwPartyInfo = PARTY_STATE_MASTER;
			lpPartyMaster = this;
			lpPartyPlayers[cnt] = this;
			LevelAvg += smCharInfo.Level;
		}
		else
		{
			lpPlayInfo = srFindUserFromSerial(lpTransPartyPlayInfo->PlayInfo[cnt].dwObjectSerial);
			if (lpPlayInfo)
			{
				lpPartyPlayers[cnt] = lpPlayInfo;
				lpPlayInfo->dwPartyInfo = PARTY_STATE_MEMBER;
				lpPlayInfo->lpPartyMaster = this;
				LevelAvg += lpPlayInfo->smCharInfo.Level;
			}
		}
	}

	//파티원 수와 평균레벨 기록
	PartyUserCount = lpTransPartyPlayInfo->PartyUserCount;
	if (PartyUserCount) PartyLevelAverage = LevelAvg / PartyUserCount;

	return TRUE;
}



//레벨 차이별 경험치 획득
int srGetTotalExp(int Exp, int Level);

// 박재원 - 파티 경험치 획득량 수정
int	PartyExpTable[6] = {
	{	100	 },
	{	80	 },
	{	73	 },
	{	70	 },
	{	68	},
	{	66	},
};

//파티원 경험치 획득
int rsPLAYINFO::GetPartyExp(smCHAR *lpChar, rsPLAYINFO *lpPlayInfo)
{
	if (dwPartyInfo != PARTY_STATE_MASTER || !lpsmSock) return FALSE;

	smTRANS_COMMAND Packet = { 0 };
	Packet.size = sizeof(smTRANS_COMMAND);
	Packet.LParam = lpChar->dwObjectSerial;

	int Exp = srGetTotalExp(lpChar->smMonsterInfo.GetExp, lpChar->smCharInfo.Level - PartyLevelAverage);
	int Experience = 0;

	std::vector<rsPLAYINFO*> Players = std::vector<rsPLAYINFO*>();
	int X = 0, Z = 0, Dist = 0;

	// Verificar se todos os membros s�o ating�veis.
	for (int i = 0; i < PARTY_PLAYER_MAX; i++)
	{
		if (lpPartyPlayers[i] && lpPartyPlayers[i]->lpsmSock)
		{
			X = (lpPlayInfo->Position.x - lpPartyPlayers[i]->Position.x) >> FLOATNS;
			Z = (lpPlayInfo->Position.z - lpPartyPlayers[i]->Position.z) >> FLOATNS;
			Dist = X*X + Z*Z;
			if (Dist < PARTY_GETTING_DIST)
			{
				Players.push_back(lpPartyPlayers[i]);
			}
		}
	}

	// Pega a experi�ncia dividida.
	Experience = (Exp * PartyExpTable[Players.size() - 1]) / 100;

	int ExpUp = 0;
	int NewExp = Experience;

	// Aplicar o b�nus dos premiuns e enviar.
	for (auto &Player : Players)
	{
		ExpUp = 0;
		NewExp = Experience;

		if (Player->dwTime_PrimeItem_ExpUp>(DWORD)tServerTime)
		{
			switch (Player->dwPrimeItem_PackageCode)
			{
				case PRIME_ITEM_PACKAGE_NONE:			ExpUp += 30;	break;
				case PRIME_ITEM_PACKAGE_BRONZE:			ExpUp += 10;	break;
				case PRIME_ITEM_PACKAGE_SILVER:			ExpUp += 20;	break;
				case PRIME_ITEM_PACKAGE_GOLD:			ExpUp += 30;	break;
				case PRIME_ITEM_PACKAGE_ULTRA:			ExpUp += 50;	break;
				case PRIME_ITEM_PACKAGE_NONE_50_EXPUP:	ExpUp += 50;	break;
				case PRIME_ITEM_PACKAGE_NONE_100_EXPUP:	ExpUp += 100;	break;
			}
		}
		if (ExpUp)
			NewExp += (Experience * ExpUp) / 100;

		Packet.code = smTRANSCODE_ADDEXP_PARTY;
		Packet.WParam = NewExp;
		Player->spEXP_Out += Packet.WParam;
		Packet.SParam = (Packet.WParam * 3 + Packet.LParam * 13) * 2002;
		Packet.EParam = Packet.SParam^Player->spEXP_Out;

		Player->lpsmSock->Send2((char *)&Packet, Packet.size, TRUE);
	}

	return TRUE;
}

//파티원 돈 획득
int rsPLAYINFO::GetPartyMoney(sITEMINFO *lpDefItemInfo, rsPLAYINFO *lpPlayInfo)
{
	TRANS_ITEMINFO	TransItemInfo;
	int	PartyMoney[2];
	int	cnt;
	int x, z, dist;
	int Sucess = 0;

	if (dwPartyInfo != PARTY_STATE_MASTER || !lpsmSock) return FALSE;

	PartyMoney[1] = lpDefItemInfo->Money / PartyUserCount;					//파티원 획득
	PartyMoney[0] = PartyMoney[1] + (lpDefItemInfo->Money%PartyUserCount);	//본인 획득


	TransItemInfo.code = smTRANSCODE_PUTITEM;
	TransItemInfo.size = sizeof(TRANS_ITEMINFO);
	memcpy(&TransItemInfo.Item, lpDefItemInfo, sizeof(sITEMINFO));
	//lpsmSock->Send( (char *)&TransItemInfo , TransItemInfo.size , TRUE );


	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt]->lpsmSock)
		{

			if (lpPartyPlayers[cnt] == lpPlayInfo)
			{
				wsprintf(TransItemInfo.Item.ItemName, sinGold, PartyMoney[0]);
				TransItemInfo.Item.Money = PartyMoney[0];
				ReformItem(&TransItemInfo.Item);						//아이템 인증
				Sucess++;
			}
			else
			{
				x = (lpPlayInfo->Position.x - lpPartyPlayers[cnt]->Position.x) >> FLOATNS;
				z = (lpPlayInfo->Position.z - lpPartyPlayers[cnt]->Position.z) >> FLOATNS;
				dist = x*x + z*z;
				if (dist < PARTY_GETTING_DIST)
				{			//파티 공유 제한 거리
					wsprintf(TransItemInfo.Item.ItemName, sinGold, PartyMoney[1]);
					TransItemInfo.Item.Money = PartyMoney[1];
					ReformItem(&TransItemInfo.Item);						//아이템 인증
				}
				else
				{
					TransItemInfo.Item.Money = 0;
				}
			}

			if (TransItemInfo.Item.Money)
			{
				lpPartyPlayers[cnt]->spMoney_Out += TransItemInfo.Item.Money;

				TransItemInfo.x = dwPlayServTime;											//해독 키
				TransItemInfo.y = TransItemInfo.Item.ItemHeader.dwChkSum;					//해독 키2
				TransItemInfo.z = lpPartyPlayers[cnt]->spMoney_Out ^ (TransItemInfo.x + TransItemInfo.y);	//토탈 기록 전송

				rsRegist_ItemSecCode(lpPartyPlayers[cnt], &TransItemInfo, 0);	//아이템 정보송수신 구조체에 보안코드 삽입 ( 컴파일시 서버에서만 존재 )

				lpPartyPlayers[cnt]->lpsmSock->Send2((char *)&TransItemInfo, TransItemInfo.size, TRUE);
			}

		}
	}
	if (!Sucess)	return FALSE;

	return TRUE;
}

//파티원 채팅메세지 보냄
int rsPLAYINFO::PartyChatting(rsPLAYINFO *lpPlayInfo)
{
	int sLen, cnt;
	TRANS_CHATMESSAGE	TransChatMessage;


	sLen = lstrlen(lpPlayInfo->szChatMessage) + 1;
	TransChatMessage.code = smTRANSCODE_WHISPERMESSAGE;
	TransChatMessage.size = 32 + lstrlen(lpPlayInfo->szChatMessage);
	TransChatMessage.dwIP = 4;			//파랑글씨 (귓말과 같다 )
	TransChatMessage.dwObjectSerial = lpPlayInfo->dwObjectSerial;
	memcpy(TransChatMessage.szMessage, lpPlayInfo->szChatMessage, sLen);

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt] && lpPartyPlayers[cnt]->lpsmSock && lpPartyPlayers[cnt] != lpPlayInfo)
		{

			lpPartyPlayers[cnt]->lpsmSock->Send((char *)&TransChatMessage, TransChatMessage.size, TRUE);
		}
	}


	return TRUE;
}


//파티장 이임
int	rsPLAYINFO::LeavePartyMaster(rsPLAYINFO *lpPlayInfo)
{
	int cnt;
	char szBuff[128];

	if (dwPartyInfo != PARTY_STATE_MASTER) return FALSE;
	if (lpPlayInfo == this) return FALSE;

	dwPartyInfo = PARTY_STATE_MEMBER;

	if (!lpPlayInfo)
	{
		//자동 이임 
		for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
		{
			if (lpPartyPlayers[cnt] && lpPartyMaster != lpPartyPlayers[cnt])
			{
				lpPlayInfo = lpPartyPlayers[cnt];
				break;
			}
		}
	}

	if (!lpPlayInfo) return FALSE;
	if (!lpPlayInfo->lpsmSock) return FALSE;

	//파티원 정보
	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt])
		{

			lpPartyPlayers[cnt]->lpPartyMaster = lpPlayInfo;		//파티장 포인터 변경
			lpPartyPlayers[cnt]->PartyMasterObjectCode = lpPlayInfo->dwObjectSerial;
			if (lpPartyPlayers[cnt] == lpPlayInfo)
			{
				lpPartyPlayers[cnt]->dwPartyInfo = PARTY_STATE_MASTER;
			}
		}
	}

	memcpy(lpPlayInfo->lpPartyPlayers, lpPartyPlayers, sizeof(rsPLAYINFO *) *PARTY_PLAYER_MAX);
	memcpy(lpPlayInfo->dwPartyObjectCode, dwPartyObjectCode, sizeof(DWORD) *PARTY_PLAYER_MAX);

	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		lpPartyPlayers[cnt] = 0;
		dwPartyObjectCode[0] = 0;
	}

	lpPlayInfo->UpdatePartyPlayer();


	//wsprintf( szBuff , "동료장이 %s님으로 변경되었습니다" , lpPlayInfo->smCharInfo.szName );
	wsprintf(szBuff, srPartyMsg1, lpPlayInfo->smCharInfo.szName);
	lpPlayInfo->SendPartyMessage(szBuff, 2);

	return TRUE;
}

//파티 해산
int	rsPLAYINFO::ReleaseParty()
{
	int cnt;
	smTRANS_COMMAND		TransCommand;

	if (lpPartyMaster != this) return FALSE;

	//SendPartyMessage( "동료가 해산되었습니다" , 2 );
	SendPartyMessage(srPartyMsg2, 2);

	dwPartyInfo = PARTY_STATE_NONE;

	TransCommand.code = smTRANSCODE_PARTY_RELEASE;
	TransCommand.size = sizeof(smTRANS_COMMAND);
	TransCommand.WParam = dwObjectSerial;
	TransCommand.LParam = 0;
	TransCommand.SParam = 0;

	//파티원 정보
	for (cnt = 0; cnt < PARTY_PLAYER_MAX; cnt++)
	{
		if (lpPartyPlayers[cnt])
		{

			lpPartyPlayers[cnt]->dwPartyInfo = PARTY_STATE_NONE;
			lpPartyPlayers[cnt]->lpPartyMaster = NULL;

			if (lpPartyPlayers[cnt]->lpsmSock)
				lpPartyPlayers[cnt]->lpsmSock->Send((char *)&TransCommand, TransCommand.size, TRUE);

			lpPartyPlayers[cnt] = 0;
		}
	}


	return TRUE;
}


//파티 가입
int	rsPLAYINFO::JoinParty(rsPLAYINFO *lpPlayMaster)
{
	char	szBuff[128];

	if (dwPartyInfo) return FALSE;

	if (lpPlayMaster && lpPlayMaster->lpsmSock && lpPlayMaster->dwPartyInfo == PARTY_STATE_MASTER)
	{

		//파티에 가입
		if (lpPlayMaster->JoinPartyPlayer(this) == TRUE)
		{
			//성공
			//wsprintf( szBuff , "%s님이 동료에 가입 했습니다" , smCharInfo.szName );
			wsprintf(szBuff, srPartyMsg3, smCharInfo.szName);
			SendPartyMessage(szBuff, 2);
		}
		else
		{
			//실패
			//wsprintf( szBuff , "동료에 가입 할수 없습니다" );
			wsprintf(szBuff, srPartyMsg4);
			SendPartyMessage(szBuff, 2);
		}
	}

	return TRUE;
}


//파티 탈퇴
int	rsPLAYINFO::SecedeParty()
{
	char	szBuff[128];

	rsPLAYINFO	*lpPlayInfo;


	if (dwPartyInfo && lpPartyMaster && lpPartyMaster->lpsmSock && lpPartyMaster->dwPartyInfo == PARTY_STATE_MASTER)
	{
		//파티에서 탈퇴

		if (this == lpPartyMaster)
		{
			LeavePartyMaster(0);		//파티장 이임
		}

		lpPlayInfo = lpPartyMaster;

		if (lpPlayInfo->PartyUserCount <= 2)
		{
			//wsprintf( szBuff , "%s님 동료에서 탈퇴했습니다" , smCharInfo.szName );
			wsprintf(szBuff, srPartyMsg5, smCharInfo.szName);
			lpPlayInfo->SendPartyMessage(szBuff, 2);

			//파티원 없음 파티 해산
			lpPlayInfo->ReleaseParty();
			return TRUE;
		}

		if (lpPlayInfo->DeletePartyPlayer(this))
		{
			//wsprintf( szBuff , "%s님 동료에서 탈퇴했습니다" , smCharInfo.szName );
			wsprintf(szBuff, srPartyMsg5, smCharInfo.szName);
			lpPlayInfo->SendPartyMessage(szBuff, 2);

			return TRUE;
		}
	}


	return FALSE;
}

rsPLAYINFO *LastDisPlayer;
rsPLAYINFO *LastDisPartyMaster;
DWORD		LastDisPartyInfo;

//파티원 연결 끈어짐
int	rsPLAYINFO::DisconnectPartyUser()
{
	char szBuff[128];
	rsPLAYINFO *pMaster;

	if (!lpPartyMaster || !dwPartyInfo) return NULL;

	LastDisPlayer = this;
	LastDisPartyMaster = lpPartyMaster;
	LastDisPartyInfo = dwPartyInfo;



	pMaster = lpPartyMaster;

	if (this == lpPartyMaster)
	{
		LeavePartyMaster(0);		//파티장 이임
	}


	//wsprintf( szBuff , "%s님 동료에서 탈퇴했습니다" , smCharInfo.szName );
	wsprintf(szBuff, srPartyMsg5, smCharInfo.szName);
	lpPartyMaster->SendPartyMessage(szBuff, 2);

	if (lpPartyMaster->PartyUserCount <= 2)
	{
		lpPartyMaster->ReleaseParty();		//파티원 없음 파티 해산
		return TRUE;
	}

	//파티에서 탈퇴
	lpPartyMaster->DeletePartyPlayer(this);


	lpPartyMaster = pMaster;

	return TRUE;
}


//////////////////////// 서버 머니 관리 ///////////////////////
/*
	int	AddServerMoney( int Money , int WhereParam=0 );		//서버 머니 추가
	int	SubServerMoney( int Money , int WhereParam=0 );		//서버 머니 감소
	int	SetServerMoney( int Money , int WhereParam=0 );		//서버 머니 설정
*/

/*
#define	WHERE_TRADE				300
#define	WHERE_GIFT_EXPRESS		600
#define	WHERE_AGINGITEM			400
#define	WHERE_CRAFTITEM			420
#define	WHERE_FORECORB			440
#define	WHERE_BUY_ITEM			200
#define	WHERE_BUY_POTION		210
#define	WHERE_SELL_ITEM			220
#define	WHERE_LOAD_PLAYER		100
#define	WHERE_THROW_ITEM		120
#define	WHERE_GET_ITEM			130
#define	WHERE_STAR_POINT		620
#define	WHERE_CLAN_MONEY		630
#define	WHERE_GIVE_MONEY		640
#define	WHERE_PERSONAL_SHOP		320
#define	WHERE_OPEN_WAREHOUES	500
*/

char *GetWhereParamString(int WareParam)
{
	char *lpString;

	lpString = "Unknow";

#ifdef _W_SERVER

	switch (WareParam)
	{

		case	WHERE_TRADE:
			lpString = "Trade";	break;
		case	WHERE_GIFT_EXPRESS:
			lpString = "GiftExpress";	break;
		case	WHERE_AGINGITEM:
			lpString = "AgingItem";	break;
		case	WHERE_CRAFTITEM:
			lpString = "CraftItem";	break;
		case	WHERE_FORECORB:
			lpString = "ForceOrb";	break;
		case	WHERE_BUY_ITEM:
			lpString = "BuyItem";	break;
		case	WHERE_BUY_POTION:
			lpString = "BuyPotion";	break;
		case	WHERE_SELL_ITEM:
			lpString = "SellItem";	break;
		case	WHERE_LOAD_PLAYER:
			lpString = "LoadPlayer";	break;
		case	WHERE_THROW_ITEM:
			lpString = "ThrowItem";	break;
		case	WHERE_GET_ITEM:
			lpString = "GetItem";	break;
		case	WHERE_STAR_POINT:
			lpString = "StarPoint";	break;
		case	WHERE_CLAN_MONEY:
			lpString = "ClanMoney";	break;
		case	WHERE_GIVE_MONEY:
			lpString = "GiveMoney";	break;
		case	WHERE_PERSONAL_SHOP:
			lpString = "PersonalShop";	break;
		case	WHERE_OPEN_WAREHOUES:
			lpString = "WareHouse";	break;
	}
#endif

	return lpString;
}


#define	CHK_SERVER_MONEY_MAX	100000000
#define	CHK_SERVER_MONEY_MAX2	150000000
#define	CHK_SERVER_MONEY_MIN	-150000000

int	rsPLAYINFO::AddServerMoney(int Money, int WhereParam)	//서버 머니 추가
{
	int money = ServerMoney;

	ServerMoney += Money;

	if ((money >= 0 && ServerMoney < 0) || Money<0 || Money>CHK_SERVER_MONEY_MAX)
	{
		//돈 오버 플로우 ( 로그 남기기 )

		smTRANS_COMMAND_EX	smTransCommandEx;

		char szStrBuff[128];
		wsprintf(szStrBuff, "*ADD_MONEY [%s]*", GetWhereParamString(WhereParam));

		smTransCommandEx.WParam = 8840;
		smTransCommandEx.WxParam = WhereParam;
		smTransCommandEx.LxParam = (int)szStrBuff;
		smTransCommandEx.LParam = money;
		smTransCommandEx.SParam = Money;
		smTransCommandEx.EParam = ServerMoney;
		RecordHackLogFile(this, &smTransCommandEx);

		if (ServerMoney < CHK_SERVER_MONEY_MIN)
			ServerMoney = CHK_SERVER_MONEY_MIN;

		return FALSE;
	}

	return TRUE;
}

int	rsPLAYINFO::SubServerMoney(int Money, int WhereParam)	//서버 머니 감소
{
	int money = ServerMoney;

	ServerMoney -= Money;

	if ((money >= 0 && ServerMoney < 0) || Money<0 || Money>CHK_SERVER_MONEY_MAX)
	{
		//돈 오버 플로우 ( 로그 남기기 )

		smTRANS_COMMAND_EX	smTransCommandEx;

		char szStrBuff[128];
		wsprintf(szStrBuff, "*SUB_MONEY [%s]*", GetWhereParamString(WhereParam));

		smTransCommandEx.WParam = 8840;
		smTransCommandEx.WxParam = WhereParam;
		smTransCommandEx.LxParam = (int)szStrBuff;
		smTransCommandEx.LParam = money;
		smTransCommandEx.SParam = Money;
		smTransCommandEx.EParam = ServerMoney;
		RecordHackLogFile(this, &smTransCommandEx);

		if (ServerMoney < CHK_SERVER_MONEY_MIN)
			ServerMoney = CHK_SERVER_MONEY_MIN;


		return FALSE;
	}

	return TRUE;
}

int	rsPLAYINFO::SetServerMoney(int Money, int WhereParam)	//서버 머니 설정
{
	ServerMoney = Money;

	if (Money > CHK_SERVER_MONEY_MAX || Money < 0)
	{
		//돈 오버 플로우 ( 로그 남기기 )

		smTRANS_COMMAND_EX	smTransCommandEx;

		char szStrBuff[128];
		wsprintf(szStrBuff, "*SET_MONEY [%s]*", GetWhereParamString(WhereParam));

		smTransCommandEx.WParam = 8840;
		smTransCommandEx.WxParam = WhereParam;
		smTransCommandEx.LxParam = (int)szStrBuff;
		smTransCommandEx.LParam = 0;
		smTransCommandEx.SParam = Money;
		smTransCommandEx.EParam = ServerMoney;
		RecordHackLogFile(this, &smTransCommandEx);

		if (ServerMoney < CHK_SERVER_MONEY_MIN)
			ServerMoney = CHK_SERVER_MONEY_MIN;

		if (ServerMoney > CHK_SERVER_MONEY_MAX2)
			ServerMoney = 0;

		return FALSE;
	}

	return TRUE;
}


int	rsPLAYINFO::CallBack_ClanMarkNum(int ClanMarkNum)			//클랜마크 번호 알려줌
{
	DWORD dwCode;
	smTRANS_COMMAND	smTransCommand;

	if (ClanMarkNum == -1) return TRUE;

	dwCode = GetClanCode(ClanMarkNum);
	if (dwClanCode != dwCode)
	{

		smTransCommand.WParam = 8870;
		smTransCommand.LParam = dwClanCode;
		smTransCommand.SParam = dwCode;
		smTransCommand.EParam = rsBlessCastle.dwMasterClan;
		RecordHackLogFile(this, &smTransCommand);

		SucessCheck_ClanCode = -1;
		//클랜 마크 코드가 틀리다
		return FALSE;
	}

	dwClanCode = dwCode;
	SucessCheck_ClanCode = TRUE;

	return TRUE;
}
