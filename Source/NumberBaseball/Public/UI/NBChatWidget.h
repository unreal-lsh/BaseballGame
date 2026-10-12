#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "NBChatWidget.generated.h"

class UEditableTextBox;
class UScrollBox;
class UTextBlock;

/**
 * 채팅 위젯의 C++ 부모 클래스. (WBP_NBChat의 부모)
 * 위젯 블루프린트에는 아래 BindWidget 변수와 같은 이름의 위젯을 배치해야 한다.
 */
UCLASS(Abstract)
class NUMBERBASEBALL_API UNBChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 채팅 기록에 한 줄을 추가한다. */
	void AddChatMessage(const FString& Message);

	/** 입력창에 키보드 포커스를 준다. */
	void FocusInput();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 입력창에서 Enter를 누르면 호출된다. */
	UFUNCTION()
	void OnChatInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** 채팅 기록이 쌓이는 스크롤 박스 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ChatScrollBox;

	/** 채팅/숫자를 입력하는 입력창 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ChatInputBox;

	/** 내 시도 횟수 "[현재 / 최대]" 표시 (없어도 됨) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GuessCountText;

	/** 보관할 채팅 줄 수. 넘치면 오래된 줄부터 지운다. */
	UPROPERTY(EditDefaultsOnly, Category = "Chat", meta = (ClampMin = "1"))
	int32 MaxChatLines = 100;

	UPROPERTY(EditDefaultsOnly, Category = "Chat", meta = (ClampMin = "1"))
	int32 ChatFontSize = 16;
};
