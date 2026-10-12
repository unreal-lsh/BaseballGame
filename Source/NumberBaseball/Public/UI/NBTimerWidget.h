#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NBTimerWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 턴 타이머 위젯의 C++ 부모 클래스. (WBP_NBTimer의 부모)
 * 스스로 시간을 세지 않고, 서버가 관리하는 GameState의 복제 값을 매 프레임 읽어 표시한다.
 * 그래서 모든 플레이어가 같은 시간을 보게 된다.
 */
UCLASS(Abstract)
class NUMBERBASEBALL_API UNBTimerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 남은 시간(초) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimerText;

	/** 누구의 차례인지 / 현재 게임 상태 (없어도 됨) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TurnText;

	/** 남은 시간 게이지 (없어도 됨) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> TimerBar;

private:
	void SetTexts(const FString& InTimerText, const FString& InTurnText, float InBarPercent);
};
