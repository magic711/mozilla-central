/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsAsyncDOMEvent.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsGUIEvent.h"

nsAsyncDOMEvent::nsAsyncDOMEvent(nsINode *aEventNode, nsEvent &aEvent)
  : mEventNode(aEventNode), mDispatchChromeOnly(false)
{
  MOZ_ASSERT(mEventNode);
  nsEventDispatcher::CreateEvent(aEventNode, nullptr, &aEvent, EmptyString(),
                                 getter_AddRefs(mEvent));
  NS_ASSERTION(mEvent, "Should never fail to create an event");
  mEvent->DuplicatePrivateData();
  mEvent->SetTrusted(aEvent.mFlags.mIsTrusted);
}

NS_IMETHODIMP nsAsyncDOMEvent::Run()
{
  if (mEvent) {
    NS_ASSERTION(!mDispatchChromeOnly, "Can't do that");
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mEventNode);
    bool defaultActionEnabled; // This is not used because the caller is async
    target->DispatchEvent(mEvent, &defaultActionEnabled);
  } else {
    nsIDocument* doc = mEventNode->OwnerDoc();
    if (mDispatchChromeOnly) {
      nsContentUtils::DispatchChromeEvent(doc, mEventNode, mEventType,
                                          mBubbles, false);
    } else {
      nsContentUtils::DispatchTrustedEvent(doc, mEventNode, mEventType,
                                           mBubbles, false);
    }
  }

  return NS_OK;
}

nsresult nsAsyncDOMEvent::PostDOMEvent()
{
  return NS_DispatchToCurrentThread(this);
}

void nsAsyncDOMEvent::RunDOMEventWhenSafe()
{
  nsContentUtils::AddScriptRunner(this);
}

nsLoadBlockingAsyncDOMEvent::~nsLoadBlockingAsyncDOMEvent()
{
  if (mBlockedDoc) {
    mBlockedDoc->UnblockOnload(true);
  }
}
