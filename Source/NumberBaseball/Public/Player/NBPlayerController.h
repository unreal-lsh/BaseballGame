#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NBPlayerController.generated.h"

class UNBChatWidget;
class UNBNoticeWidget;
class UNBTimerWidget;

/**
 * 클라이언트와 서버를 잇는 창구.
 * - 클라이언트 → 서버 : ServerSendChatMessage (채팅/숫자 입력 전달)
 * - 서버 → 해당 클라이언트 : ClientReceiveSystemMessage (본인에게만 보이는 안내)
 * 또한 로컬 플레이어의 위젯(채팅, 공지, 타이머)을 생성하고 보관한다.
 */
UCLASS()
class NUMBERBASEBALL_API ANBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANBPlayerController();

	/** 입력창에 적은 내용을 서버로 보낸다. */
	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(const FString& Message);

	/** 서버가 이 플레이어에게만 보내는 안내 메시지 (잘못된 입력, 차례 아님 등) */
	UFUNCTION(Client, Reliable)
	void ClientReceiveSystemMessage(const FString& Message);

	/** 로컬 채팅창에 한 줄을 추가한다. */
	void AddChatMessage(const FString& Message);

	/** 로컬 공지 위젯을 Duration초 동안 띄운다. */
	void ShowNotice(const FString& Message, float Duration);

protected:
	virtual void BeginPlay() override;

private:
	void CreateLocalWidgets();

	/** 채팅 입력창에 포커스를 준다. 위젯이 화면에 붙은 다음 프레임에 호출한다. */
	void FocusChatInput();

	/** 사용할 위젯 블루프린트. 기본값은 /Game/UI 아래의 WBP 경로이며 에디터에서 바꿀 수 있다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UNBChatWidget> ChatWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UNBNoticeWidget> NoticeWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UNBTimerWidget> TimerWidgetClass;

	UPROPERTY()
	TObjectPtr<UNBChatWidget> ChatWidget;

	UPROPERTY()
	TObjectPtr<UNBNoticeWidget> NoticeWidget;

	UPROPERTY()
	TObjectPtr<UNBTimerWidget> TimerWidget;
};
