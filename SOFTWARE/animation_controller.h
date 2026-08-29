#ifndef ANIMATION_CONTROLLER_H
#define ANIMATION_CONTROLLER_H

#include "animation.h"
#include "action.h"
#include <memory>

/**
 * @brief Owns and advances one LED strip's current animation and action queue.
 *
 * @details Concurrency contract: update() is called from the repeating-timer
 *          ISR while every other mutator (play, enqueue, stop,
 *          setCurrentAnimation) is called from the main loop. The mutators
 *          mask interrupts around their critical sections so the ISR never
 *          observes a half-modified controller.
 *
 *          Nothing in this class allocates or frees memory in ISR context:
 *          the queue is a fixed ring buffer, and objects the ISR finishes
 *          with are parked on a retired list instead of being destroyed.
 *          The main loop must call reap() regularly (pack_state_process()
 *          does, as do the blocking sequence helpers) to actually destroy
 *          retired objects. That deferral is also what makes it safe for the
 *          main loop to hold a pointer from getCurrentAnimation() while the
 *          ISR finishes that animation: the object stays alive until the
 *          next reap() on the same (main) context.
 */
class AnimationController {
public:
    AnimationController();
    ~AnimationController();

    void play(std::unique_ptr<Action> action);
    void play(std::unique_ptr<Animation> anim, const AnimationConfig& config);
    void enqueue(std::unique_ptr<Action> action);
    void update(uint32_t dt);
    void stop();
    bool isRunning() const;

    /** @brief Destroys retired objects. Main-loop context only. */
    void reap();

    Animation* getCurrentAnimation();
    void setCurrentAnimation(std::unique_ptr<Animation> anim);

private:
    /** Pending actions beyond this are refused (and freed by the caller). */
    static constexpr int kQueueCapacity = 8;
    /** Finished objects awaiting reap(); sized so it cannot fill between
     *  reap() calls in practice. */
    static constexpr int kRetiredCapacity = 16;

    // The *Locked helpers assume interrupts are already masked (or that the
    // caller is the ISR itself, which cannot be preempted by the main loop).
    void startNextActionLocked();
    void clearQueueLocked();
    void retireLocked(std::unique_ptr<Action> action);
    void retireLocked(std::unique_ptr<Animation> anim);

    std::unique_ptr<Action> actionQueue[kQueueCapacity];
    int queueHead = 0;
    int queueCount = 0;

    std::unique_ptr<Action> currentAction;
    std::unique_ptr<Animation> currentAnimation;

    std::unique_ptr<Action> retiredActions[kRetiredCapacity];
    int retiredActionCount = 0;
    std::unique_ptr<Animation> retiredAnimations[kRetiredCapacity];
    int retiredAnimationCount = 0;
};

#endif // ANIMATION_CONTROLLER_H
