// FaP All Rights Reserve

#include "DataAssets/Combat/EnemyCombatPositionProfile.h"

#include "Misc/DataValidation.h"

EDataValidationResult UEnemyCombatPositionProfile::IsDataValid(
	FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context),
		EDataValidationResult::Valid);

	auto ValidateDistanceRange = [&Context, &Result](
		const TCHAR* ZoneName,
		const float MinDistance,
		const float MaxDistance)
	{
		if (MaxDistance < MinDistance)
		{
			Context.AddError(FText::Format(
				NSLOCTEXT(
					"EnemyCombatPositionProfile",
					"InvalidDistanceRange",
					"{0}: 最远距离不能小于最近距离。"),
				FText::FromString(ZoneName)));
			Result = EDataValidationResult::Invalid;
		}
	};

	ValidateDistanceRange(TEXT("前方区域"), FrontMinDistance, FrontMaxDistance);
	ValidateDistanceRange(TEXT("待机圆环"), IdleMinDistance, IdleMaxDistance);
	ValidateDistanceRange(TEXT("屏幕外区域"), OffscreenMinDistance, OffscreenMaxDistance);
	return Result;
}

