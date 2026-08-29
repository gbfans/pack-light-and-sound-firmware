#include "animation_controller.h"

// Interrupt masking: real on the Pico so main-loop mutators exclude the
// timer ISR; a no-op in the host-side simulator, which is single threaded.
#ifdef PICO_BUILD
#include "hardware/sync.h"
typedef uint32_t ctrl_irq_state_t;
static inline ctrl_irq_state_t ctrl_lock(void) {
    return save_and_disable_interrupts();
}
static inline void ctrl_unlock(ctrl_irq_state_t state) {
    restore_interrupts(state);
}
#else
typedef int ctrl_irq_state_t;
static inline ctrl_irq_state_t ctrl_lock(void) { return 0; }
static inline void ctrl_unlock(ctrl_irq_state_t) {}
#endif

AnimationController::AnimationController() : currentAction(nullptr), currentAnimation(nullptr) {}

AnimationController::~AnimationController() {
    stop();
    reap();
}

void AnimationController::play(std::unique_ptr<Action> action) {
    stop();
    enqueue(std::move(action));
}

void AnimationController::play(std::unique_ptr<Animation> anim, const AnimationConfig& config) {
    play(std::make_unique<PlayAnimationAction>(std::move(anim), config));
}

void AnimationController::enqueue(std::unique_ptr<Action> action) {
    if (!action) {
        return;
    }
    ctrl_irq_state_t irq = ctrl_lock();
    if (queueCount < kQueueCapacity) {
        actionQueue[(queueHead + queueCount) % kQueueCapacity] = std::move(action);
        queueCount++;
        if (!currentAction) {
            startNextActionLocked();
        }
    }
    ctrl_unlock(irq);
    // If the queue was full, `action` still owns the object and destroys it
    // here, in the caller's (main-loop) context.
}

void AnimationController::update(uint32_t dt) {
    // ISR context. The main loop cannot preempt this, and its mutators mask
    // interrupts, so no lock is taken here. Finished objects are retired,
    // never destroyed: destruction is heap work, and the heap is not
    // reentrant against the main loop.
    if (currentAction) {
        if (currentAction->update(dt)) {
            retireLocked(std::move(currentAction));
            startNextActionLocked();
        }
    }
    if (currentAnimation) {
        currentAnimation->update(dt);
        if (currentAnimation->isDone()) {
            retireLocked(std::move(currentAnimation));
        }
    }
}

void AnimationController::stop() {
    ctrl_irq_state_t irq = ctrl_lock();
    clearQueueLocked();
    if (currentAction) {
        // If an action ever stops its own controller from inside start(),
        // retiring (rather than deleting) keeps the object alive until the
        // next reap(), so the in-progress call cannot free its own `this`.
        retireLocked(std::move(currentAction));
    }
    if (currentAnimation) {
        retireLocked(std::move(currentAnimation));
    }
    ctrl_unlock(irq);
}

bool AnimationController::isRunning() const {
    // Report running status if an action is currently executing,
    // if an animation is actively playing, or if there are pending
    // actions in the queue. This ensures higher-level code can wait
    // for animations like the powercell waterfall to complete before
    // proceeding.
    return currentAction != nullptr || currentAnimation != nullptr || queueCount > 0;
}

void AnimationController::reap() {
    // Move retired objects out under the lock, then let them destroy on
    // function exit - outside the critical section, in main-loop context.
    std::unique_ptr<Action> deadActions[kRetiredCapacity];
    std::unique_ptr<Animation> deadAnimations[kRetiredCapacity];
    ctrl_irq_state_t irq = ctrl_lock();
    for (int i = 0; i < retiredActionCount; i++) {
        deadActions[i] = std::move(retiredActions[i]);
    }
    retiredActionCount = 0;
    for (int i = 0; i < retiredAnimationCount; i++) {
        deadAnimations[i] = std::move(retiredAnimations[i]);
    }
    retiredAnimationCount = 0;
    ctrl_unlock(irq);
}

Animation* AnimationController::getCurrentAnimation() {
    return currentAnimation.get();
}

void AnimationController::setCurrentAnimation(std::unique_ptr<Animation> anim) {
    ctrl_irq_state_t irq = ctrl_lock();
    if (currentAnimation) {
        retireLocked(std::move(currentAnimation));
    }
    currentAnimation = std::move(anim);
    ctrl_unlock(irq);
}

void AnimationController::startNextActionLocked() {
    if (queueCount > 0) {
        currentAction = std::move(actionQueue[queueHead]);
        queueHead = (queueHead + 1) % kQueueCapacity;
        queueCount--;
        currentAction->start(this);
    }
}

void AnimationController::clearQueueLocked() {
    while (queueCount > 0) {
        retireLocked(std::move(actionQueue[queueHead]));
        queueHead = (queueHead + 1) % kQueueCapacity;
        queueCount--;
    }
}

void AnimationController::retireLocked(std::unique_ptr<Action> action) {
    if (retiredActionCount < kRetiredCapacity) {
        retiredActions[retiredActionCount++] = std::move(action);
    } else {
        // Can't destroy here (possibly ISR context) and nowhere to park it.
        // Unreachable while reap() runs every main-loop pass; leaking is the
        // only corruption-free fallback.
        action.release();
    }
}

void AnimationController::retireLocked(std::unique_ptr<Animation> anim) {
    if (retiredAnimationCount < kRetiredCapacity) {
        retiredAnimations[retiredAnimationCount++] = std::move(anim);
    } else {
        anim.release();
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
    (void)dt;
    return true;
}
