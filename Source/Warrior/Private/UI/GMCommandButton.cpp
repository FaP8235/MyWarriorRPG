// FaP All Rights Reserve

#include "UI/GMCommandButton.h"
#include "UI/GMPanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UGMCommandButton::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &UGMCommandButton::HandleClicked);
	}
}

void UGMCommandButton::SetLabelText(const FString& Text)
{
	if (LabelText)
	{
		LabelText->SetText(FText::FromString(Text));
	}
}

void UGMCommandButton::HandleClicked()
{
	if (OwnerPanel)
	{
		OwnerPanel->ExecuteCommand(CommandIndex);
	}
}
