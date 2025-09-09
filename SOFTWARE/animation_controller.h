#ifndef ANIMATION_CONTROLLER_H
#define ANIMATION_CONTROLLER_H

#include "animation.h"
#include "action.h"
#include <queue>
#include <memory>

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

    Animation* getCurrentAnimation();
    void setCurrentAnimation(std::unique_ptr<Animation> anim);

private:
    void startNextAction();
    void clearQueue();

    std::queue<std::unique_ptr<Action>> actionQueue;
    std::unique_ptr<Action> currentAction;
    std::unique_ptr<Animation> currentAnimation;
};

#endif // ANIMATION_CONTROLLER_H
