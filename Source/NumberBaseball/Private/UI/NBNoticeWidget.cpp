#include "UI/NBNoticeWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UNBNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
}

void UNBNoticeWidget::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	Super::NativeDestruct();
}

void UNBNoticeWidget::ShowNotice(const FString& Message, float Duration)
{
	if (NoticeText != nullptr)
	{
		NoticeText->SetText(FText::FromString(Message));
	}

	// 공지가 떠 있어도 아래 위젯(채팅 입력창)을 클릭할 수 있도록 히트 테스트는 받지 않는다.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HideTimerHandle, this, &UNBNoticeWidget::HideNotice, FMath::Max(Duration, 0.1f), false);
	}
}

void UNBNoticeWidget::HideNotice()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
