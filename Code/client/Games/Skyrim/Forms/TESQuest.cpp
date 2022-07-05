#include <Forms/TESQuest.h>

#include <Services/PapyrusService.h>

#include <Events/TopicEvent.h>

#include <Games/Overrides.h>
#include <Games/ActorExtension.h>

TESObjectREFR* TESQuest::GetAliasedRef(uint32_t aAliasID) noexcept
{
    TP_THIS_FUNCTION(TGetAliasedRef, BSPointerHandle<TESObjectREFR>*, TESQuest, BSPointerHandle<TESObjectREFR>*, uint32_t);
    POINTER_SKYRIMSE(TGetAliasedRef, getAliasedRef, 25066);

    BSPointerHandle<TESObjectREFR> result{};
    ThisCall(getAliasedRef, this, &result, aAliasID);

    return TESObjectREFR::GetByHandle(result.handle.iBits);
}

TESQuest::State TESQuest::getState()
{
    if (flags >= 0)
    {
        if (unkFlags)
            return State::WaitingPromotion;
        else if (flags & 1)
            return State::Running;
        else
            return State::Stopped;
    }

    return State::WaitingForStage;
}

void TESQuest::SetCompleted(bool force)
{
    TP_THIS_FUNCTION(TSetCompleted, void, TESQuest, bool);
    POINTER_SKYRIMSE(TSetCompleted, SetCompleted, 24991);
    SetCompleted(this, force);
}

void TESQuest::CompleteAllObjectives()
{
    TP_THIS_FUNCTION(TCompleteAllObjectives, void, TESQuest);
    POINTER_SKYRIMSE(TCompleteAllObjectives, CompleteAll, 23231);
    CompleteAll(this);
}

void TESQuest::SetActive(bool toggle)
{
    if (toggle)
        flags |= 0x800;
    else
        flags &= 0xF7FF;
}

bool TESQuest::IsStageDone(uint16_t stageIndex)
{
    for (Stage* it : stages)
    {
        if (it->stageIndex == stageIndex)
            return it->IsDone();
    }

    return false;
}

bool TESQuest::Kill()
{
    using TSetStopped = void(TESQuest*, bool);
    POINTER_SKYRIMSE(TSetStopped, SetStopped, 24987);

    if (flags & Flags::Enabled)
    {
        unkFlags = 0;
        flags = Flags::Completed;
        MarkChanged(2);

        //SetStopped(this, false);
        return true;
    }

    return false;
}

bool TESQuest::EnsureQuestStarted(bool &success, bool force)
{
    TP_THIS_FUNCTION(TSetRunning, bool, TESQuest, bool*, bool);
    POINTER_SKYRIMSE(TSetRunning, SetRunning, 25003);
    return SetRunning(this, &success, force);
}

bool TESQuest::SetStage(uint16_t newStage)
{
    ScopedQuestOverride _;

    TP_THIS_FUNCTION(TSetStage, bool, TESQuest, uint16_t);
    POINTER_SKYRIMSE(TSetStage, SetStage, 25004);
    return SetStage(this, newStage);
}

void TESQuest::ScriptSetStage(uint16_t stageIndex)
{
    for (Stage* stage : stages)
    {
        if (stage->stageIndex == stageIndex && stage->IsDone())
            return;
    }

    using Quest = TESQuest;
    PAPYRUS_FUNCTION(void, Quest, SetCurrentStageID, int);
    s_pSetCurrentStageID(this, stageIndex);
}

void TESQuest::SetStopped()
{
    flags &= 0xFFFE;
    MarkChanged(2);
}

using TProcessTopicEvent = void(void*, TESTopicInfoEvent*);
static TProcessTopicEvent* RealProcessTopicEvent = nullptr;

void TESQuest::ExecuteDialogueFragment(TESTopicInfoEvent* aEvent) noexcept
{
    void* pVM = (void*)0x142FC2A90;
    ThisCall(RealProcessTopicEvent, pVM, aEvent);
}

void TP_MAKE_THISCALL(HookProcessTopicEvent, void, TESTopicInfoEvent* apEvent)
{
    if (apEvent->pSpeaker && apEvent->pSpeaker->formID != 0x14)
    {
        if (Actor* pActor = Cast<Actor>(apEvent->pSpeaker))
        {
            auto* pExtension = pActor->GetExtension();
            if (pExtension->IsLocal())
                World::Get().GetDispatcher().trigger(TopicEvent(pActor->formID, apEvent->topicId1, apEvent->eType, apEvent->topicId2));
        }
    }

    return ThisCall(RealProcessTopicEvent, apThis, apEvent);
}

static TiltedPhoques::Initializer s_questInitHooks([]() { 
    POINTER_SKYRIMSE(TProcessTopicEvent, processEvent, 53997);

    RealProcessTopicEvent = processEvent.Get();

    TP_HOOK(&RealProcessTopicEvent, HookProcessTopicEvent);
});
