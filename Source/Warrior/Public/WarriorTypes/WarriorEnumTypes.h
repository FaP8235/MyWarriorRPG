#pragma once
UENUM()
enum class EWarriorConfirmType : uint8
{
	Yes,
	No
};

UENUM()
enum class EWarriorValidType : uint8
{
	Valid,
	InValid
};

UENUM()
enum class EWarriorSuccessType : uint8
{
	Successful,
	Faild
};

UENUM()
enum class EWarriorCountDownActionInput : uint8
{
	Start,
	Cancel
};

UENUM()
enum class EWarriorCountDownActionOutput : uint8
{
	Updated,
	Completed,
	Cancelled
};

UENUM()
enum class EWarriorGameDifficulty : uint8
{
	Easy,
	Normal,
	Hard,
	VeryHard
};

UENUM(BlueprintType)
enum class EWarriorInputMode : uint8
{
	GameOnly,
	UIOnly
};