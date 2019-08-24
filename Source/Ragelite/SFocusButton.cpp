#include "SFocusButton.h"

FReply SFocusButton::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	auto Reply = SWidget::OnFocusReceived(MyGeometry, InFocusEvent);
	
	OnFocused.ExecuteIfBound();

	return Reply;
}

void SFocusButton::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	SWidget::OnFocusLost(InFocusEvent);

	OnUnfocused.ExecuteIfBound();
}
