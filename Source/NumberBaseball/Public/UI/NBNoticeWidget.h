#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "NBNoticeWidget.generated.h"

class UTextBlock;

/**
 * 게임 결과(승리/무승부)를 알리는 공지 위젯의 C++ 부모 클래스. (WBP_NBNotice의 부모)
 * 평소에는 숨겨져 있다가 ShowNotice가 호출되면 지정한 시간 동안만 보인다.
 */
UCLASS(Abstract)
class NUMBERBASEBALL_API UNBNoticeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Message를 Duration초 동안 화면에 띄운다. */
	void ShowNotice(const FString& Message, float Duration);

	void HideNotice();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 공지 문구 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NoticeText;

private:
	FTimerHandle HideTimerHandle;
};
