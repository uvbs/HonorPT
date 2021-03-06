/*----------------------------------------------------------------------------*
*	파일명 :  sinInterFace.cpp
*	하는일 :  인터페이스에 관한 모든걸 관리한다
*	작성일 :  최종업데이트 12월
*	적성자 :  박상열
*-----------------------------------------------------------------------------*/

#include "sinLinkHeader.h"
#include "..\\tjboy\\clanmenu\\tjclan.h"
#include "..\\field.h"
/*----------------------------------------------------------------------------*
*								전역변수
*-----------------------------------------------------------------------------*/
cINTERFACE	cInterFace;
sSHADOWSTATE sShadowState[3];

int sinBmpLength[5] = { 0,0,0,0,0 }; //라이프 마나 스테미나 경험치 

////////////////채팅관련 Flag
int sinChatEnter = 0;         //채팅 플랙 (엔터키를 입력하면 채팅 창이 나온다 )
int SubChatHeight = 0;	      //채팅창 변경 위치 좌표 값 	
int sinChatDisplayMode = 0;   //현재 채팅의 모드 
int sinChatInputMode = 0;     //채팅 입력 모드 
////////////////

/////////////Test Key Msg
int TestKeyAniFlag = 0;

////////////////버튼관련 Flag
int ExitButtonClick = 0;
int HelpButtonClick = 0;
int MouseButtonPosi = 0;
int sinMenuBottonPosi = 0;
int sinRunCameraMapPosi = 0; //걷기 카메라 맵 
int SunandMoonFlag[2] = { 0,0 }; //0 해 1 달 
int InterButtonDown = 0;  //인터페이스 버튼 다운 플랙 
int sinFireShow = 1;      //불을 보여준다 

////////////////스킬 게이지 (현재는 사용되고 있지 않다 )
int ReSizelpGage = 0;     //게이지 그림을 조정한다 


////////////////
int InitStateFlag = 0;    //초기화 플랙 


//////////////// 백업 포인터 
char szInfoMsgBuff5[512];  //생명력 기력 근력 경험치 의 수치 기록 버퍼 
char szInfoMsgBuff6[128];  //생명력 기력 근력 경험치 의 수치 기록 버퍼 
POINT InfoMessageBoxPos;  //텍스를 표시할 좌표 


//////////////// 전사 (다시시작 )
int ReStartFlag = 0;  //값이 들어오면 다시시작 인터페이스를 보여준다 
POINT ReStartMainXY = { 0,0 }; //다시시작 좌표 
int ReStartIndex = 0;
POINT ReStartTextXY = { 0,0 };
char szReStartMsg[128];
char szReStartMsg2[128];

int RestartCheckExp = 0;
int ReStartOptionIndex = 0;
int ShowExpPercentFlag = 0;

//////////////// 스킬 전업 종료  (다시시작 )

int SkillNpcFlag = 0;  //값이 들어오면 다시시작 인터페이스를 보여준다 
POINT SkillMasterMainXY = { 0,0 }; //다시시작 좌표 
int SkillMasterIndex = 0;
POINT SkillMasterTextXY = { 0,0 };
char szSkillMasterMsg[128];
char szSkillMasterMsg2[128];
int SkillMasterOptionIndex = 0;

////////스킬 사용 불충족 이펙트 
int NotUseSkillEffect[3] = { 0,0,0 };

int SkillMouseButtonPosi[2] = { 0,0 };

/////////// 낮 밤
int DayOrNightFlag = 0;
int sinGageTime2 = 0;


int ReStartButtonRect[3][4] = {
	{21,22,21 + 44,22 + 44},			//필드에서 시작 
	{72,22,72 + 44,22 + 44},			//마을에서 시작 
	{123,22,123 + 44,22 + 44},			//게임 종료 

};

int ButtonRect[6][5] = {   //박스 오픈 버튼 
	{648,560,648 + 25,560 + 27},
	{673,560,673 + 25,560 + 27},
	{698,560,698 + 25,560 + 27},
	{723,560,723 + 25,560 + 27},
	{748,560,748 + 25,560 + 27},
	{772,560,772 + 25,560 + 27},

};
/*
int ButtonRect[6][5]= {   //박스 오픈 버튼
	{655,566,678,598},
	{678,566,701,598},
	{701,566,724,598},
	{724,566,747,598},
	{747,566,770,598},
	{770,566,792,598},

};
*/
int sinMenuRect[3][4] = {  //게임 종료 ....
	{692,492,784,511},
	{692,517,784,536},
	{692,542,784,561}, //종료 


};

int sinRunCameraMap[3][4] = { //걷기 , 카메라 , 맵 
	{569,555,569 + 26,555 + 26},
	{569 + 26,555,569 + 26 + 26,555 + 26},
	{569 + 26 + 26,555,569 + 26 + 26 + 26,555 + 26},

};


// 전업 관련 
int ChangeJobButtonclick = 0;


//PK관련
int sinPKWarningExpFlag = 0;
int sinPKWarnigExpTime = 0;
LPDIRECTDRAWSURFACE4 lpGrowInterLeft;
LPDIRECTDRAWSURFACE4 lpGrowInterMiddle;
LPDIRECTDRAWSURFACE4 lpGrowInterRight;


//글자 이펙트관련 ===================================================================
struct HAEFFECT
{
	int   Flag;
	int   Time;
	POINT Posi;
	POINT Size;
	int   Mat;
	int   Alpha;
};
char *HaEffectFilePath[] = {
	0,
	"Image\\SinImage\\Inter\\defense.tga", //1
	"Image\\SinImage\\Inter\\block.tga",   //2
	"Image\\SinImage\\Inter\\Evade.tga",   //3
	0,
};
#define  MAX_HAEFFECTNUM 10 //총갯수

int HA_EffectIndex[4] = { 0, }; //

HAEFFECT HaEffect[MAX_HAEFFECTNUM];

int DefaultLength[4] = { 94,94,76,86 };
float sinOldNowState[4] = { 0,0,0,0 };
float sinOldMaxState[4] = { 0,0,0,0 };
char InputSecretCode[256];
char sinBuff[256];
int  ResetQuset3Flag = 0;
DWORD dwCheckTime; //시간을 체크한다 
int   CheckNotUseSkillTime[3] = { 0,0,0 };
int   CheckNotUseSkillCount[3] = { 0,0,0 };
int   CheckTime2[3] = { 0,0,0 };
int   CheckExpFlag = 0;
int   CheckExpFlag2 = 0;
int   CheckExpTime = 0;
DWORD dwTestKeyTime = 0;
int   TeskKeyMessageCnt = 0;
int sinWinSizeX[] = { 1024,1280,1600 };
int sinWinSizeXindex[] = { 2,6,11 };
int nCheckSkillDraw = 0;		//해외
char *szTestKeyMsg[6] = { "나!  &(^_^)&    ",
"노!  /(^_^)/   ",
"나!  *(^_^)*  ",
"",
"",
"", };

cINTERFACE::cINTERFACE()
{

}
cINTERFACE::~cINTERFACE()
{

}
void cINTERFACE::Init()
{
	Log::Debug("Init : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::Load()
{
	Log::Debug("Load : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::Release()
{
	Log::Debug("Release : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::Draw()
{
	Log::Debug("Draw : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::Main()
{
	Log::Debug("Main : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::Close()
{
	Log::Debug("Close : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::LButtonDown(int x, int y)
{
	Log::Debug("LButtonDown : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::LButtonUp(int x, int y)
{
	Log::Debug("LButtonUp : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::RButtonDown(int x, int y)
{
	Log::Debug("RButtonDown : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::RButtonUp(int x, int y)
{

}
void cINTERFACE::KeyDown()
{
	Log::Debug("KeyDown : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::InitState()
{
	Log::Debug("InitState : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::CheckingState()
{
	Log::Debug("CheckingState : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::ShowParaState()
{
	Log::Debug("ShowParaState : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::DrawInterText()
{
	Log::Debug("DrawInterText : Requisitado por�m foi removido.");
	return;
}
void cINTERFACE::CheckAllBox(int ButtonFlag)
{
	Log::Debug("CheckAllBox : Requisitado por�m foi removido.");
	return;
}
int cINTERFACE::DrawClockArrow(int x, int y, int Angle)
{
	Log::Debug("DrawClockArrow : Requisitado por�m foi removido.");
	return NULL;
}
int cINTERFACE::DrawCompass(int x, int y, int Angle)
{
	Log::Debug("DrawCompass : Requisitado por�m foi removido.");
	return NULL;
}
int cINTERFACE::CheckExpPercentControl()
{
	Log::Debug("CheckExpPercentControl : Requisitado por�m foi removido.");
	return NULL;
}
int cINTERFACE::GetWordTimeDisplay()
{
	Log::Debug("GetWordTimeDisplay : Requisitado por�m foi removido.");
	return NULL;
}
void cINTERFACE::ShowExpPercent()
{
	Log::Debug("ShowExpPercent : Requisitado por�m foi removido.");
	return;
}
int cINTERFACE::SetStringEffect(int Index)
{
	Log::Debug("SetStringEffect : Requisitado por�m foi removido.");
	return NULL;
}