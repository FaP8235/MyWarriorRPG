// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GMCommandButton.generated.h"

class UButton;
class UTextBlock;
class UGMPanelWidget;

/**
 * GM 面板的单个指令按钮（C++ 基类）：存 index + 路由点击到 OwnerPanel。
 * BP 只管视觉（Button + TextBlock 样式）。
 */
UCLASS(Abstract)
class WARRIOR_API UGMCommandButton : public UUserWidget
{
	GENERATED_BODY()

public:
	int32 CommandIndex = INDEX_NONE;
	TObjectPtr<UGMPanelWidget> OwnerPanel = nullptr;

	virtual void NativeConstruct() override;
	void SetLabelText(const FString& Text);

	/** BP 里 BindWidget 绑名为 Button 的 UButton。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	/** BP 里 BindWidget 绑名为 LabelText 的 UTextBlock。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

private:
	UFUNCTION()
	void HandleClicked();
};
