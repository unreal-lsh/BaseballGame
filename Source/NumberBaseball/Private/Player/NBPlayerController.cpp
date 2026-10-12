#include "Player/NBPlayerController.h"
#include "NumberBaseball.h"
#include "Game/NBGameModeBase.h"
#include "UI/NBChatWidget.h"
#include "UI/NBNoticeWidget.h"
#include "UI/NBTimerWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	/** 소프트 클래스 경로의 위젯 블루프린트를 불러와 화면에 붙인다. 없으면 경고만 남기고 nullptr을 돌려준다. */
	template <typename WidgetType>
	WidgetType* CreateWidgetFromSoftClass(APlayerController* OwningPlayer, const TSoftClassPtr<WidgetType>& SoftClass, int32 ZOrder)
	{
		UClass* WidgetClass = SoftClass.LoadSynchronous();
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogNumberBaseball, Warning, TEXT("위젯 블루프린트를 찾을 수 없습니다: %s (README의 위젯 블루프린트 만들기를 확인하세요)"), *SoftClass.ToString());
			return nullptr;
		}

		WidgetType* Widget = CreateWidget<WidgetType>(OwningPlayer, WidgetClass);
		if (Widget != nullptr)
		{
			Widget->AddToViewport(ZOrder);
		}
		return Widget;
	}
}

ANBPlayerController::ANBPlayerController()
{
	ChatWidgetClass = TSoftClassPtr<UNBChatWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_NBChat.WBP_NBChat_C")));
	NoticeWidgetClass = TSoftClassPtr<UNBNoticeWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_NBNotice.WBP_NBNotice_C")));
	TimerWidgetClass = TSoftClassPtr<UNBTimerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_NBTimer.WBP_NBTimer_C")));
}

void ANBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 위젯은 화면을 가진 로컬 플레이어에게만 필요하다. (서버가 가진 남의 컨트롤러에는 만들지 않는다)
	if (IsLocalController())
	{
		CreateLocalWidgets();

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;

		// 위젯이 실제로 화면에 붙은 뒤에야 포커스를 줄 수 있으므로 다음 프레임으로 미룬다.
		GetWorldTimerManager().SetTimerForNextTick(this, &ANBPlayerController::FocusChatInput);
	}
}

void ANBPlayerController::CreateLocalWidgets()
{
	TimerWidget = CreateWidgetFromSoftClass(this, TimerWidgetClass, 0);
	ChatWidget = CreateWidgetFromSoftClass(this, ChatWidgetClass, 1);
	NoticeWidget = CreateWidgetFromSoftClass(this, NoticeWidgetClass, 10);
}

void ANBPlayerController::FocusChatInput()
{
	if (ChatWidget != nullptr)
	{
		ChatWidget->FocusInput();
	}
}

void ANBPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	// 여기부터는 서버에서 실행된다. 규칙 판단은 전부 GameMode에 맡긴다.
	if (ANBGameModeBase* GameMode = GetWorld()->GetAuthGameMode<ANBGameModeBase>())
	{
		GameMode->HandleChatMessage(this, Message);
	}
}

void ANBPlayerController::ClientReceiveSystemMessage_Implementation(const FString& Message)
{
	AddChatMessage(FString::Printf(TEXT("[안내] %s"), *Message));
}

void ANBPlayerController::AddChatMessage(const FString& Message)
{
	if (ChatWidget != nullptr)
	{
		ChatWidget->AddChatMessage(Message);
	}
}

void ANBPlayerController::ShowNotice(const FString& Message, float Duration)
{
	if (NoticeWidget != nullptr)
	{
		NoticeWidget->ShowNotice(Message, Duration);
	}
}
