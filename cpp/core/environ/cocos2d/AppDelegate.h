#pragma once
#include <cocos2d.h>

class TVPAppDelegate : public cocos2d::Application {

    void initGLContextAttrs() override;

    /**
     * @brief    Implement Director and Scene init code here.
     * @return true    Initialize success, app continue.
     * @return false   Initialize failed, app terminate.
     */
    bool applicationDidFinishLaunching() override;

    /**
     * @brief  The function be called when the application enter background
     */
    void applicationDidEnterBackground() override;

    /**
     * @brief  The function be called when the application enter foreground
     */
    void applicationWillEnterForeground() override;

    void applicationScreenSizeChanged(int newWidth, int newHeight);
};