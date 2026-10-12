#include "UI/NBChatWidget.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

void UNBChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChatInputBox != nullptr)
	{
		ChatInputBox->OnTextCommitted.AddUniqueDynamic(this, &UNBChatWidget::OnChatInputCommitted);

		// Enter를 눌러도 포커스가 유지되어 연속으로 입력할 수 있게 한다.
		ChatInputBox->SetClearKeyboardFocusOnCommit(false);
	}
}

void UNBChatWidget::NativeDestruct()
{
	if (ChatInputBox != nullptr)
	{
		ChatInputBox->OnTextCommitted.RemoveDynamic(this, &UNBChatWidget::OnChatInputCommitted);
	}

	Super::NativeDestruct();
}

void UNBChatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 시도 횟수는 PlayerState에서 복제된 값을 Getter로 읽어 온다.
	if (GuessCountText != nullptr)
	{
		const APlayerController* OwningPlayer = GetOwningPlayer();
		const ANBPlayerState* MyPlayerState = OwningPlayer != nullptr ? OwningPlayer->GetPlayerState<ANBPlayerState>() : nullptr;
		if (MyPlayerState != nullptr)
		{
			GuessCountText->SetText(FText::FromString(FString::Printf(TEXT("시도 횟수 %s"), *MyPlayerState->GetGuessCountString())));
		}
	}
}

void UNBChatWidget::OnChatInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	const FString Message = Text.ToString().TrimStartAndEnd();
	if (!Message.IsEmpty())
	{
		// 위젯은 화면만 담당한다. 입력 내용은 PlayerController의 Server RPC로 서버에 넘긴다.
		if (ANBPlayerController* OwningController = Cast<ANBPlayerController>(GetOwningPlayer()))
		{
			OwningController->ServerSendChatMessage(Message);
		}
	}

	if (ChatInputBox != nullptr)
	{
		ChatInputBox->SetText(FText::GetEmpty());
	}
}

void UNBChatWidget::FocusInput()
{
	if (ChatInputBox != nullptr)
	{
		ChatInputBox->SetKeyboardFocus();
	}
}

void UNBChatWidget::AddChatMessage(const FString& Message)
{
	if (ChatScrollBox == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	UTextBlock* ChatLine = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (ChatLine == nullptr)
	{
		return;
	}

	FSlateFontInfo FontInfo = ChatLine->GetFont();
	FontInfo.Size = ChatFontSize;
	ChatLine->SetFont(FontInfo);
	ChatLine->SetAutoWrapText(true);
	ChatLine->SetText(FText::FromString(Message));

	ChatScrollBox->AddChild(ChatLine);

	while (ChatScrollBox->GetChildrenCount() > MaxChatLines)
	{
		ChatScrollBox->RemoveChildAt(0);
	}

	ChatScrollBox->ScrollToEnd();
}
