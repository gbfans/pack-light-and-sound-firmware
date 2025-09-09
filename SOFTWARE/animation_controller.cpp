#include "animation_controller.h"

AnimationController::AnimationController() : currentAction(nullptr), currentAnimation(nullptr) {}

AnimationController::~AnimationController() {
    stop();
}

void AnimationController::play(std::unique_ptr<Action> action) {
    stop();
    enqueue(std::move(action));
}

void AnimationController::play(std::unique_ptr<Animation> anim, const AnimationConfig& config) {
    play(std::make_unique<PlayAnimationAction>(std::move(anim), config));
}

void AnimationController::enqueue(std::unique_ptr<Action> action) {
    actionQueue.push(std::move(action));
    if (!currentAction) {
        startNextAction();
    }
}

void AnimationController::update(uint32_t dt) {
    if (currentAction) {
        if (currentAction->update(dt)) {
            currentAction.reset();
            startNextAction();
        }
    }
    if (currentAnimation) {
        currentAnimation->update(dt);
        if (currentAnimation->isDone()) {
            currentAnimation.reset();
        }
    }
}

void AnimationController::stop() {
    clearQueue();
    currentAction.reset();
    currentAnimation.reset();
}

bool AnimationController::isRunning() const {
    // Report running status if an action is currently executing,
    // if an animation is actively playing, or if there are pending
    // actions in the queue. This ensures higher-level code can wait
    // for animations like the powercell waterfall to complete before
    // proceeding.
    return currentAction != nullptr || currentAnimation != nullptr || !actionQueue.empty();
}

Animation* AnimationController::getCurrentAnimation() {
    return currentAnimation.get();
}

void AnimationController::setCurrentAnimation(std::unique_ptr<Animation> anim) {
    currentAnimation = std::move(anim);
}

void AnimationController::startNextAction() {
    if (!actionQueue.empty()) {
        currentAction = std::move(actionQueue.front());
        actionQueue.pop();
        currentAction->start(this);
    }
}

void AnimationController::clearQueue() {
    while (!actionQueue.empty()) {
        actionQueue.pop();
    }
}

// Implementation of PlayAnimationAction
void PlayAnimationAction::start(AnimationController* controller) {
    Action::start(controller);
    controller->setCurrentAnimation(std::move(animation));
    controller->getCurrentAnimation()->start(config);
}

bool PlayAnimationAction::update(uint32_t dt) {
    // This action is done in start()
    return true;
}
