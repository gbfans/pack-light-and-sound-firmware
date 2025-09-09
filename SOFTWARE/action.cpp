#include "action.h"
#include "animation_controller.h"

void ChangeColorAction::start(AnimationController* controller) {
    Action::start(controller);
    Animation* anim = controller->getCurrentAnimation();
    if (anim) {
        anim->setColor(color, duration_ms, mode);
    }
}

bool ChangeColorAction::update(uint32_t dt) {
    // This action is done in start()
    return true;
}

void ChangeSpeedAction::start(AnimationController* controller) {
    Action::start(controller);
    Animation* anim = controller->getCurrentAnimation();
    if (anim) {
        anim->setSpeed(speed, duration_ms, mode);
    }
}

bool ChangeSpeedAction::update(uint32_t dt) {
    // This action is done in start()
    return true;
}
