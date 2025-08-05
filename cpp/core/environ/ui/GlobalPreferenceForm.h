#pragma once
#include "PreferenceForm.h"

class TVPGlobalPreferenceForm : public TVPPreferenceForm {
public:
    static TVPGlobalPreferenceForm *
    create(const tPreferenceScreen *config = nullptr);
    static void Initialize();
protected:
    bool initFromBuilder(const Csd::NodeBuilderFn &naviBarBuilder,
                         const Csd::NodeBuilderFn &bodyBuilder,
                         const Csd::NodeBuilderFn &bottomBarBuilder, Node *parent) override;

};