// FaP All Rights Reserve


#include "Controllers/WarriorHeroController.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/GMPanelWidget.h"

AWarriorHeroController::AWarriorHeroController()
{
    HeroTeamID = FGenericTeamId(0);
}

FGenericTeamId AWarriorHeroController::GetGenericTeamId() const
{
    return HeroTeamID;
}

void AWarriorHeroController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (GMPanelInputAction)
        {
            EIC->BindAction(GMPanelInputAction, ETriggerEvent::Started, this, &AWarriorHeroController::ToggleGMPanel);
        }
    }
}

void AWarriorHeroController::ToggleGMPanel()
{
    if (GMPanelInstance)
    {
        GMPanelInstance->RemoveFromParent();
        GMPanelInstance = nullptr;
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
    }
    else if (GMPanelWidgetClass)
    {
        GMPanelInstance = CreateWidget<UGMPanelWidget>(GetWorld(), GMPanelWidgetClass);
        if (GMPanelInstance)
        {
            GMPanelInstance->AddToViewport(100);
            bShowMouseCursor = true;
            SetInputMode(FInputModeGameAndUI());
        }
    }
}
