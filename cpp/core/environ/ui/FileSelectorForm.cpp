#include "FileSelectorForm.h"
#include "StorageImpl.h"
#include "ui/UIListView.h"
#include "ui/UIHelper.h"
#include "ui/UIButton.h"
#include "ui/UIText.h"
#include "ui/UITextField.h"
#include "ui/UICheckBox.h"
#include "Platform.h"
#include "cocos2d/MainScene.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "platform/CCFileUtils.h"
#include "base/CCDirector.h"
#include "MessageBox.h"
#include "platform/CCDevice.h"
#include "base/CCScheduler.h"
#include "utils/TickCount.h"
#include <condition_variable>
#include <filesystem>

#include "csd/CsdUIFactory.h"

using namespace cocos2d;
using namespace cocos2d::extension;
using namespace cocos2d::ui;

extern std::thread::id TVPMainThreadID;

const char *const FileName_Cell = "ui/FileItem.csb";
static TVPListForm *_listform;
#define MOVE_INCH (7.0f / 160.0f)
static const std::string str_long_press("long_press");

static float convertDistanceFromPointToInch(const Vec2 &dis) {
    auto glview = cocos2d::Director::getInstance()->getOpenGLView();
    int dpi = cocos2d::Device::getDPI();
    float distance = Vec2(dis.x * glview->getScaleX() / dpi,
                          dis.y * glview->getScaleY() / dpi)
                         .getLength();
    return distance;
}

static bool IsPathExist(const std::string &path) {
    tTVP_stat s;
    if(!TVP_stat(path.c_str(), s)) {
        return false; // not exist
    }
    return true;
}

// ptah 传入要求utf-8编码的路径
// 返回值：父路径和文件名
std::pair<std::string, std::string>
TVPBaseFileSelectorForm::PathSplit(const std::string &path) {
    std::filesystem::path p;
    try {
        #ifdef _WIN32
            // 1. UTF-8 → UTF-16
            int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
            if (wlen <= 0) return { "", "" };
            std::wstring wpath(wlen - 1, L'\0');           // 去掉末尾 '\0'
            MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wpath.data(), wlen - 1);

            p = std::filesystem::path{ wpath };
        #else
            // Linux/macOS 直接构造即可
           p = std::filesystem::path{ path };
        #endif
    } catch (const std::system_error& e) {
        spdlog::error("Invalid path: {}", e.what());
        spdlog::error("Path: {}", path);
        return { "", "" };    
    }

    // 获取父路径和文件名
#ifdef _WIN32
    std::string parent   = p.parent_path().u8string();
    std::string filename = p.filename().u8string();
#else
    std::string parent   = p.parent_path().string();
    std::string filename = p.filename().string();
#endif

    return { parent, filename };
}

TVPBaseFileSelectorForm::TVPBaseFileSelectorForm() : FileList(nullptr) {
    std::vector<std::string> paths;
    getShortCutDirList(paths); // for init
    std::vector<std::string> appPath = TVPGetAppStoragePath();
    if(appPath.empty() && paths.size() == 1) { // ios-like sandbox environment
        RootPathLen = paths.front().size();
    }
}

TVPBaseFileSelectorForm::~TVPBaseFileSelectorForm() {
    CC_SAFE_RELEASE_NULL(CellTemplateForSize);
}

void TVPBaseFileSelectorForm::bindHeaderController(const Node *allNodes) {
    _title = allNodes->getChildByName<Button *>("title");
    if(_title) {
        _title->setEnabled(true);
        _title->addClickEventListener([this](auto &&PH1) {
            onTitleClicked(std::forward<decltype(PH1)>(PH1));
        });
    }
}

void TVPBaseFileSelectorForm::bindBodyController(const Node *allNodes) {
    Node *TableNode = allNodes->getChildByName("table");
    const auto &size = TableNode->getContentSize();
    CCASSERT(size.width > 0 && size.height > 0,
             "TableNode content size is invalid");
    FileList = TableView::create(this, size);
    FileList->setVerticalFillOrder(TableView::VerticalFillOrder::TOP_DOWN);
    FileList->setClippingToBounds(true);
    FileList->setTouchEnabled(true);
    FileList->setPosition(Vec2::ZERO);
    FileList->setDirection(cocos2d::extension::ScrollView::Direction::VERTICAL);
    FileList->setBounceable(true); // 允许滚动时有弹性
    TableNode->addChild(FileList);

    if(NaviBar.Left) {
        NaviBar.Left->addClickEventListener(
            [this](Ref *r) { onBackClicked(r); });
    }
}

static const std::string str_diricon("dir_icon");
static const std::string str_select("select_check");
static const std::string str_filename("filename");

std::string utf8_to_local(const std::string &utf8) {
    #if defined(_WIN32)
    // to UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return "";
    std::wstring wstr(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wstr.data(), wlen - 1);

    // to（GBK/ANSI）
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string local(len - 1, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, local.data(), len - 1, nullptr, nullptr);

    return local;
    
    #elif defined(__LINUX__) || defined(__APPLE__)
    // Linux/macOS 直接返回原始字符串
    return utf8;
    #endif
}

std::wstring local_to_wstr(const std::string &local) {
    #ifdef _WIN32
    // to UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, local.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring wstr(wlen - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, local.c_str(), -1, wstr.data(), wlen - 1);

    return wstr;
    
    #else
    // Linux/macOS 直接转换为 wstring
    return std::wstring(local.begin(), local.end());
    #endif
}

std::wstring utf8_to_wstr(const std::string& utf8)
{

#ifdef _WIN32
    if (utf8.empty()) return {};

    // 1. 计算目标长度
    int len = MultiByteToWideChar(CP_UTF8, 0,
                                  utf8.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};

    // 2. 转换
    std::wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0,
                        utf8.c_str(), -1, &out[0], len);

    // len 包含末尾 L'\0'，去掉
    out.pop_back();
    return out;
#else 
    // Linux/macOS 直接返回原始字符串转换为 wstring
    return std::wstring(utf8.begin(), utf8.end());
#endif
}

std::string local_to_utf8(const std::string &local) {
    #ifdef _WIN32
    // to UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, local.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return "";
    std::wstring wstr(wlen - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, local.c_str(), -1, wstr.data(), wlen - 1);

    // to UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string utf8(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, utf8.data(), len - 1, nullptr, nullptr);

    return utf8;
    
    #else
    // Linux/macOS 直接返回原始字符串
    return local;
    #endif
}
void TVPBaseFileSelectorForm::ListDir(std::string path) {
    auto [fst, snd] = PathSplit(path);

    ParentPath = fst;

    if(_title) {

        const Size dispSize = _title->getContentSize();
        const size_t fontSize =
            _title->getTitleRenderer()->getRenderingFontSize();
        const size_t pathLen = snd.length();
        const ptrdiff_t overflowChar =
            (pathLen * fontSize - dispSize.width) / fontSize;
        if(overflowChar > 0) {
            constexpr char suffix[] = "...";
            const size_t renderCharCount =
                pathLen - overflowChar - std::size(suffix);
            const std::string curPath = snd.substr(0, renderCharCount);
            _title->setTitleText(curPath + suffix);
        } else {
            _title->setTitleText(snd);
        }
    }

    if(path.size() > RootPathLen && path.back() == '/') {
        path.pop_back();
    }


    if(NaviBar.Left) {
        NaviBar.Left->setVisible(path.size() > RootPathLen);
    }

    CurrentDirList.clear();
    CurrentDirList.reserve(16);
    TVPListDir(path, [&](const std::string &name, int mask) {
        if(mask & (S_IFREG | S_IFDIR)) {
            if(name.empty() || name.front() == '.')
                return;
            CurrentDirList.resize(CurrentDirList.size() + 1);
            FileInfo &info = CurrentDirList.back();
            info.NameForDisplay = name;
            info.NameForCompare = name;
            info.IsDir = S_ISDIR(mask);
            std::transform(info.NameForCompare.begin(),
                           info.NameForCompare.end(),
                           info.NameForCompare.begin(), [](int c) -> int {
                               if(c <= 'Z' && c >= 'A')
                                   return c - ('A' - 'a');
                               return c;
                           });
        }
    });

    // fill fullpath
    for(auto &it : CurrentDirList) {
        it.FullPath = path + "/" + it.NameForDisplay;
        #ifdef _DEBUG
        spdlog::info("Found file: FullPath {}, NameForDisplay {}, IsDir {}",
                     utf8_to_local(it.FullPath),  utf8_to_local(it.NameForDisplay), it.IsDir);
        #endif
    }

    std::sort(CurrentDirList.begin(), CurrentDirList.end());

    // update
    bool keepPos = CurrentPath == path;
    CurrentPath = path;
    if(keepPos) {
        ReloadTableViewAndKeepPos(FileList);
    } else {
        FileList->reloadData();
    }

    if(!_selectedFileIndex.empty()) {
        _selectedFileIndex.clear();
        updateFileMenu();
    } else if(_clipboardForFileManager.size()) {
        updateFileMenu();
    }
}

void TVPBaseFileSelectorForm::onCellClicked(int idx) {
    const FileInfo &info = CurrentDirList[idx];
    if(info.IsDir) {
        ListDir(info.FullPath);
    }
}

void TVPBaseFileSelectorForm::onCellLongPress(int idx) {
    if(_fileOperateMenuNode) { // full file function
        if(!_selectedFileIndex.empty()) {
            return;
        }
        if(!_fileOperateMenu) {
            CSBReader reader;
            _fileOperateMenu = reader.Load("ui/FileManageMenu.csb");
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("title"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleUnselect"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleView", false));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleCopy"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleCut"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titlePaste"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleUnpack"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleDelete"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleSendTo"));
            LocaleConfigManager::GetInstance()->initText(
                reader.findController<Text>("titleRename"));
            _fileOperateMenulist = reader.findController<ListView>("list");
            _fileOperateCell_unselect = reader.findWidget("unselect");
            _fileOperateCell_view = reader.findWidget("view", false);
            _fileOperateCell_copy = reader.findWidget("copy");
            _fileOperateCell_cut = reader.findWidget("cut");
            _fileOperateCell_paste = reader.findWidget("paste");
            _fileOperateCell_delete = reader.findWidget("delete");
            _fileOperateCell_unpack = reader.findWidget("unpack", false);
            _fileOperateCell_sendto = reader.findWidget("sendto");
            _fileOperateCell_rename = reader.findWidget("rename", false);
            Widget *btn;
            ;
            if((btn = reader.findWidget("btnUnselect"))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onUnselectClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnView", false))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onViewClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnCopy"))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onCopyClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnCut"))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onCutClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnPaste"))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onPasteClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnUnpack", false))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onUnpackClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnDelete"))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onDeleteClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnSendTo"))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onSendToClicked, this,
                              std::placeholders::_1));
            }
            if((btn = reader.findWidget("btnRename"))) {
                btn->addClickEventListener(
                    std::bind(&TVPBaseFileSelectorForm::onBtnRenameClicked,
                              this, std::placeholders::_1));
            }
            _fileOperateMenulist->removeAllChildrenWithCleanup(false);
            float scale = 1.5;
            _fileOperateMenu->setScale(1 / scale);
            _fileOperateMenu->setContentSize(
                _fileOperateMenuNode->getContentSize() * scale);
            _fileOperateMenu->setPosition(_fileOperateMenuNode->getPosition());
            _fileOperateMenuNode->getParent()->addChild(_fileOperateMenu);
            ui::Helper::doLayout(_fileOperateMenu);
            _fileOperateMenu->setVisible(false);
        }
        if(!_fileOperateMenu->isVisible()) {
            _fileOperateMenu->setVisible(true);
            //	_fileOperateMenu->setAnchorPoint(Vec2(1, 0));
        }

        FileList->setTouchEnabled(false); // trick to release all touches
        FileList->setTouchEnabled(true);
        _selectedFileIndex.clear();
        _selectedFileIndex.insert(idx);
        updateFileMenu();
        return;
    }
    // simple file manage
    LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
    const char *btnTitles[] = {
        localeMgr->GetText("delete").c_str(),
        localeMgr->GetText("cancel").c_str(),
    };
    int n = TVPShowSimpleMessageBox(
        localeMgr->GetText("file_operate_menu_text").c_str(),
        localeMgr->GetText("file_operate_menu_title").c_str(),
        sizeof(btnTitles) / sizeof(btnTitles[0]), btnTitles);
    const FileInfo &info = CurrentDirList[idx];
    switch(n) {
        case 0:
            _selectedFileIndex.insert(idx);
            onDeleteClicked(nullptr);
            _selectedFileIndex.clear();
            break;
        default:
            break;
    }
}

void TVPBaseFileSelectorForm::getShortCutDirList(
    std::vector<std::string> &pathlist) {
    std::vector<std::string> paths = TVPGetDriverPath();
    for(const std::string &path : paths) {
        pathlist.emplace_back(path);
    }
    std::vector<std::string> appPath = TVPGetAppStoragePath();
    for(auto path : appPath) {
        cocos2d::log("appPath: %s", path.c_str());
        pathlist.emplace_back(path);
    }
}

void TVPBaseFileSelectorForm::onTitleClicked(cocos2d::Ref *owner) {
    if(_listform)
        return;
    std::vector<std::string> paths;
    getShortCutDirList(paths);

    std::vector<Widget *> cells;
    std::vector<Button *> buttons;
    auto func = [this](cocos2d::Ref *node) {
        ListDir(dynamic_cast<Button *>(node)->getCallbackName());
        TVPMainScene::GetInstance()->popUIForm(nullptr,
                                               TVPMainScene::eLeaveToBottom);
    };
    for(const std::string &path : paths) {
        CSBReader reader;
        auto *cell = dynamic_cast<Widget *>(
            reader.Load("ui/ListItem.csb")->getChildByName("item"));
        Button *item = dynamic_cast<Button *>(reader.findController("item"));
        item->setCallbackName(path);
        item->setTitleText(path);
        item->addClickEventListener(func);
        cells.emplace_back(cell);
        buttons.emplace_back(item);
    }
    _listform = TVPListForm::create(cells);
    _listform->show();
    // march all button's text in its width
    for(Button *btn : buttons) {
        Size dispSize = btn->getTitleRenderer()->getContentSize();
        Size realSize = btn->getContentSize();
        if(dispSize.width > realSize.width) {
            std::string text = btn->getTitleText();
            float ratio = realSize.width / dispSize.width;
            const std::string prefix("...");
            int charCutCount = text.size() * (1 - ratio) + prefix.size() + 1;
            btn->setTitleText(prefix + text.substr(charCutCount));
        }
    }
}

void TVPBaseFileSelectorForm::onBackClicked(cocos2d::Ref *owner) {
    ListDir(ParentPath);
}

TVPBaseFileSelectorForm::FileItemCellImpl *
TVPBaseFileSelectorForm::FetchCell(FileItemCellImpl *CellModel,
                                   cocos2d::extension::TableView *table,
                                   ssize_t idx) {
    if(!CellModel) {
        CellModel = FileItemCellImpl::create(Csd::createFileItem,
                                             table->getViewSize().width);
        CellModel->setAnchorPoint(Vec2::ZERO);
        CellModel->setEventFunc(
            [this](Widget::TouchEventType ev, Widget *sender, Touch *touch) {
                Vec2 touchPoint = touch->getLocation();
                switch(ev) {
                    case Widget::TouchEventType::BEGAN:
                        FileList->onTouchBegan(touch, nullptr);
                        break;
                    case Widget::TouchEventType::MOVED:
                        FileList->onTouchMoved(touch, nullptr);
                        if(sender->isHighlighted() &&
                           convertDistanceFromPointToInch(
                               sender->getTouchBeganPosition() - touchPoint) >
                               MOVE_INCH) {
                            sender->setHighlighted(false);
                        }
                        break;

                    case Widget::TouchEventType::CANCELED:
                        FileList->onTouchCancelled(touch, nullptr);
                        break;
                    case Widget::TouchEventType::ENDED:
                        FileList->onTouchEnded(touch, nullptr);
                        break;
                }
            });
        CellModel->retain();
    }
    bool selected = _selectedFileIndex.find(idx) != _selectedFileIndex.end();
    CellModel->setInfo(idx, CurrentDirList[idx], selected,
                       !_selectedFileIndex.empty());
    return CellModel;
}

TableViewCell *TVPBaseFileSelectorForm::tableCellAtIndex(TableView *table,
                                                         ssize_t idx) {
    TableViewCell *pCell = table->dequeueCell();
    FileItemCell *cell = nullptr;
    if(pCell) {
        cell = dynamic_cast<FileItemCell *>(pCell);
    } else {
        cell = FileItemCell::create(this);
    }
    if(static_cast<size_t>(idx) >= CurrentDirList.size()) {
        cell->setVisible(false);
        return cell;
    }
    cell->setVisible(true);
    FileItemCellImpl *impl = cell->detach();
    cell->attach(FetchCell(impl, table, idx));
    return cell;
}

ssize_t TVPBaseFileSelectorForm::numberOfCellsInTableView(TableView *table) {
    return CurrentDirList.empty() ? 0 : CurrentDirList.size() + 1;
}

Size TVPBaseFileSelectorForm::tableCellSizeForIndex(TableView *table,
                                                    ssize_t idx) {
    if((size_t)idx >= CurrentDirList.size()) {
        return Size(table->getContentSize().width, 200);
    }
    FileInfo &info = CurrentDirList[idx];
    if(info.CellSize.width == 0.f) {
        if(!CellTemplateForSize) {
            CellTemplateForSize = FetchCell(nullptr, table, idx);
        } else {
            CellTemplateForSize->setInfo(idx, CurrentDirList[idx], false,
                                         false);
        }
        info.CellSize = CellTemplateForSize->getContentSize();
    }
    return info.CellSize;
}

void TVPBaseFileSelectorForm::rearrangeLayout() {
    iTVPBaseForm::rearrangeLayout();
    if(FileList)
        FileList->setViewSize(FileList->getParent()->getContentSize());
}

void TVPBaseFileSelectorForm::onUnselectClicked(cocos2d::Ref *owner) {
    clearFileMenu();
}

void TVPBaseFileSelectorForm::onViewClicked(cocos2d::Ref *owner) {}

void TVPBaseFileSelectorForm::onCopyClicked(cocos2d::Ref *owner) {
    _clipboardForMoving = false;
    _clipboardForFileManager.clear();
    for(int idx : _selectedFileIndex) {
        _clipboardForFileManager.emplace_back(CurrentDirList[idx].FullPath);
    }
    _clipboardPath = CurrentPath;
    _selectedFileIndex.clear();
    updateFileMenu();
}

void TVPBaseFileSelectorForm::onCutClicked(cocos2d::Ref *owner) {
    onCopyClicked(owner);
    _clipboardForMoving = true;
}

void TVPBaseFileSelectorForm::onPasteClicked(cocos2d::Ref *owner) {
    // TODO progress bar
    auto func = _clipboardForMoving ? TVPRenameFile : TVPCopyFile;
    for(const std::string &path : _clipboardForFileManager) {
        auto split_path = PathSplit(path);
        std::string target = CurrentPath + "/" + split_path.second;
        if(IsPathExist(target)) {
            LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
            if(TVPShowSimpleMessageBoxYesNo(
                   target, localeMgr->GetText("ensure_to_override_file")) !=
               0) {
                continue;
            }
        }
        if(!func(path, target)) {
            LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
            TVPShowSimpleMessageBox(target,
                                    localeMgr->GetText("file_operate_failed"));
            break;
        }
    }
    clearFileMenu();
    ListDir(CurrentPath);
}

void TVPBaseFileSelectorForm::onUnpackClicked(cocos2d::Ref *owner) {
    if(_selectedFileIndex.size() != 1) {
        return;
    }
    FileInfo &info = CurrentDirList[*_selectedFileIndex.begin()];
    _selectedFileIndex.clear();
    clearFileMenu();
    ttstr outpath = TVPChopStorageExt(info.FullPath);

    class UnpackArchive {
        const int UpdateMS = 100; // update rate 10 fps
        tTVPUnpackArchive ArcUnpacker;
        std::string ArcPath;
        std::string PasswordBuffer;

    public:
        bool Init(const std::string &path, const std::string &outpath) {
            ArcPath = path;
            ArcUnpacker.SetCallback(
                std::bind(&UnpackArchive::OnEnded, this),
                std::bind(&UnpackArchive::OnError, this, std::placeholders::_1,
                          std::placeholders::_2),
                std::bind(&UnpackArchive::OnProgress, this,
                          std::placeholders::_1, std::placeholders::_2),
                std::bind(&UnpackArchive::OnNewFile, this,
                          std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3),
                std::bind(&UnpackArchive::OnPassword, this));
            if(ArcUnpacker.Prepare(path, outpath, &TotalSize) <= 0) {
                return false;
            }
            if(TotalSize > 1024 * 1024) {
                ProgressForm = TVPSimpleProgressForm::create();
                TVPMainScene::GetInstance()->pushUIForm(
                    ProgressForm, TVPMainScene::eEnterAniNone);
                std::vector<
                    std::pair<std::string, std::function<void(cocos2d::Ref *)>>>
                    vecButtons;
                vecButtons.emplace_back("Stop",
                                        [this](Ref *) { ArcUnpacker.Stop(); });
                ProgressForm->initButtons(vecButtons);
                ProgressForm->setTitle("Unpacking...");
                ProgressForm->setPercentOnly(0);
                ProgressForm->setPercentOnly2(0);
                ProgressForm->setPercentText("");
                ProgressForm->setPercentText2("");
                ProgressForm->setContent("");
            }
            return true;
        }

        virtual ~UnpackArchive() {
            if(ProgressForm) {
                TVPMainScene::GetInstance()->popUIForm(
                    ProgressForm, TVPMainScene::eLeaveAniNone);
                ProgressForm = nullptr;
            }
        }

        void Start(const std::function<void()> &onEnded) {
            FuncOnEnded = onEnded;
            ArcUnpacker.Start();
        }

    private:
        void OnEnded() {
            Director::getInstance()
                ->getScheduler()
                ->performFunctionInCocosThread([this] {
                    if(FuncOnEnded)
                        FuncOnEnded();
                    delete this;
                });
        }

        void OnError(int err, const char *msg) {
            Director::getInstance()
                ->getScheduler()
                ->performFunctionInCocosThread([this, err, msg] {
                    char buf[64];
                    sprintf(buf, "Error %d\n", err);
                    ttstr strmsg(buf);
                    strmsg += msg;
                    TVPShowSimpleMessageBox(strmsg,
                                            TJS_W("Fail to unpack archive"));
                });
        }

        void OnProgress(tjs_uint64 total_size, tjs_uint64 file_size) {
            if(!ProgressForm)
                return;
            tjs_uint32 tick = TVPGetRoughTickCount32();
            if((int)(tick - LastUpdate) < UpdateMS) {
                return;
            }
            LastUpdate = tick;
            Director::getInstance()
                ->getScheduler()
                ->performFunctionInCocosThread([this, total_size, file_size] {
                    ProgressForm->setPercentOnly((float)total_size / TotalSize);
                    ProgressForm->setPercentOnly2((float)file_size /
                                                  CurrentFileSize);
                    char buf[64];
                    int sizeMB = static_cast<int>(total_size / (1024 * 1024)),
                        totalMB = static_cast<int>(TotalSize / (1024 * 1024));
                    sprintf(buf, "%d / %dMB", sizeMB, totalMB);
                    ProgressForm->setPercentText(buf);
                    sizeMB = static_cast<int>(file_size / (1024 * 1024));
                    totalMB = static_cast<int>(CurrentFileSize / (1024 * 1024));
                    sprintf(buf, "%d / %dMB", sizeMB, totalMB);
                    ProgressForm->setPercentText2(buf);
                });
        }

        void OnNewFile(int idx, const std::string &utf8name,
                       tjs_uint64 file_size) {
            if(!ProgressForm)
                return;
            tjs_uint32 tick = TVPGetRoughTickCount32();
            if((int)(tick - LastUpdate) < UpdateMS && file_size < 1024 * 1024) {
                return;
            }
            LastUpdate = tick;
            CurrentFileSize = file_size;
            std::string filename(utf8name);
            Director::getInstance()
                ->getScheduler()
                ->performFunctionInCocosThread(
                    [this, filename] { ProgressForm->setContent(filename); });
        }

        bool _onPassword() {
            LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
            std::vector<ttstr> btns;
            btns.emplace_back(localeMgr->GetText("ok"));
            btns.emplace_back(localeMgr->GetText("cancel"));
            ttstr text(ArcPath);
            if(TVPShowSimpleInputBox(
                   text, localeMgr->GetText("please_input_password"), "",
                   btns) == 0) {
                PasswordBuffer = text.AsStdString();
                return true;
            } else {
                ArcUnpacker.Stop();
                PasswordBuffer.clear();
                return false;
            }
        }

        std::string OnPassword() {
            if(TVPMainThreadID == std::this_thread::get_id()) {
                _onPassword();
            } else {
                std::mutex mtx;
                std::condition_variable cond;
                std::unique_lock<std::mutex> lk(mtx);
                Director::getInstance()
                    ->getScheduler()
                    ->performFunctionInCocosThread([&]() {
                        _onPassword();
                        cond.notify_all();
                    });
                cond.wait(lk);
            }
            return PasswordBuffer;
        }

        TVPSimpleProgressForm *ProgressForm = nullptr;
        tjs_uint32 LastUpdate = 0;
        tjs_uint64 TotalSize, CurrentFileSize;
        std::function<void()> FuncOnEnded;
    };
    UnpackArchive *unpacker = new UnpackArchive;
    // using libarchive to unpack archive
    if(unpacker->Init(info.FullPath, outpath.AsStdString())) {
        unpacker->Start([this]() { ListDir(CurrentPath); });
        return;
    }
    delete unpacker;

    // use internal archive module
    tTVPArchive *arc = TVPOpenArchive(info.FullPath, false);
    if(!arc) {
        LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
        TVPShowSimpleMessageBox(info.FullPath,
                                localeMgr->GetText("fail_to_open_archive"));
        return;
    }

    tjs_uint count = arc->GetCount();
    for(tjs_uint i = 0; i < count; ++i) {
        ttstr name = arc->GetName(i);
        ttstr fullpath = outpath + name;
        tTJSBinaryStream *st = arc->CreateStreamByIndex(i);
        if(!st)
            continue;
        TVPSaveStreamToFile(st, 0, st->GetSize(), fullpath);
        delete st;
    }

    delete arc;
}

void TVPBaseFileSelectorForm::onDeleteClicked(cocos2d::Ref *owner) {
    LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
    std::string content;
    if(_selectedFileIndex.size() == 1) {
        const FileInfo &info = CurrentDirList[*_selectedFileIndex.begin()];
        content = info.FullPath;
    } else {
        content = localeMgr->GetText("ensure_item_count");
        char tmp[64];
        snprintf(tmp, 64, content.c_str(), _selectedFileIndex.size());
        content = tmp;
    }
    if(TVPShowSimpleMessageBoxYesNo(
           content, localeMgr->GetText("ensure_to_delete_file")) == 0) {
        for(int idx : _selectedFileIndex) {
            TVPDeleteFile(CurrentDirList[idx].FullPath);
        }
        scheduleOnce([this](float) { ListDir(CurrentPath); }, 0,
                     "refresh_path");
    }
}

void TVPBaseFileSelectorForm::onSendToClicked(cocos2d::Ref *owner) {
    if(_selectedFileIndex.size() != 1)
        return;
    FileInfo &info = CurrentDirList[*_selectedFileIndex.begin()];
    TVPSendToOtherApp(info.FullPath);
    clearFileMenu();
}

void TVPBaseFileSelectorForm::onBtnRenameClicked(cocos2d::Ref *owner) {
    if(_selectedFileIndex.size() != 1)
        return;
    FileInfo &info = CurrentDirList[*_selectedFileIndex.begin()];
    ttstr name = info.NameForDisplay.c_str();
    std::vector<ttstr> btns;
    btns.emplace_back("OK");
    btns.emplace_back("Cancel");
    if(TVPShowSimpleInputBox(name, "Input new name", "", btns) == 0) {
        ttstr newname = TVPExtractStoragePath(info.FullPath);
        newname += name;
        std::string strNewName = newname.AsStdString();
        TVPRenameFile(info.FullPath, strNewName);
        info.FullPath = strNewName;
        info.NameForDisplay = name.AsStdString();
        ReloadTableViewAndKeepPos(FileList);
    }
    clearFileMenu();
}

void TVPBaseFileSelectorForm::updateFileMenu() {
    ReloadTableViewAndKeepPos(FileList);
    if(_selectedFileIndex.empty() &&
       _clipboardForFileManager.empty()) { // close menu
        if(_fileOperateMenu->isVisible()) {
            _fileOperateMenu->setVisible(false);
            //	_fileOperateMenu->setAnchorPoint(Vec2(0, 0));
        }
        return;
    }
    _fileOperateMenulist->removeAllChildrenWithCleanup(false);
    _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_unselect.get());
    if(!_clipboardForFileManager.empty() && _clipboardPath != CurrentPath) {
        _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_paste.get());
    }
    if(_selectedFileIndex.size() == 1) {
        if(_fileOperateCell_view)
            _fileOperateMenulist->pushBackCustomItem(
                _fileOperateCell_view.get());
        _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_unpack.get());
        _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_rename.get());
#if CC_PLATFORM_IOS == CC_TARGET_PLATFORM // TODO implement other platform
        _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_sendto.get());
#endif
    }
    if(!_selectedFileIndex.empty()) {
        _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_copy.get());
        _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_cut.get());
        _fileOperateMenulist->pushBackCustomItem(_fileOperateCell_delete.get());
    }
}

void TVPBaseFileSelectorForm::clearFileMenu() {
    _selectedFileIndex.clear();
    _clipboardForFileManager.clear();
    updateFileMenu();
}

void TVPBaseFileSelectorForm::_onCellClicked(int idx) {
    if(_selectedFileIndex.empty())
        return onCellClicked(idx);
    auto it = _selectedFileIndex.find(idx);
    if(it == _selectedFileIndex.end()) {
        _selectedFileIndex.insert(idx);
    } else {
        _selectedFileIndex.erase(idx);
    }
    updateFileMenu();
}

bool TVPBaseFileSelectorForm::FileInfo::operator<(const FileInfo &rhs) const {
    if(IsDir != rhs.IsDir)
        return IsDir > rhs.IsDir;
    return NameForCompare < rhs.NameForCompare;
}

TVPListForm *
TVPListForm::create(const std::vector<cocos2d::ui::Widget *> &cells) {
    TVPListForm *ret = new TVPListForm;
    ret->initFromInfo(cells);
    ret->autorelease();
    return ret;
}

void TVPListForm::initFromInfo(
    const std::vector<cocos2d::ui::Widget *> &cells) {
    init();
    float scale = TVPMainScene::GetInstance()->getUIScale();
    cocos2d::Size sceneSize =
        TVPMainScene::GetInstance()->getUINodeSize() / scale;
    setScale(scale);
    setContentSize(sceneSize);
    CSBReader reader;
    _root = reader.Load("ui/ListView.csb");
    ListView *listview =
        dynamic_cast<ListView *>(reader.findController("list"));
    float height = 10;
    for(Widget *cell : cells) {
        height += cell->getContentSize().height;
    }
    _root->setAnchorPoint(Size(0.5, 0.5));
    _root->setPosition(sceneSize / 2);
    sceneSize.width *= 0.8f;
    sceneSize.height *= 0.8f;
    if(height < sceneSize.height * scale) {
        sceneSize.height = height;
    }
    _root->setContentSize(sceneSize);
    ui::Helper::doLayout(_root);
    float width = listview->getContentSize().width;
    for(Widget *cell : cells) {
        Size size = cell->getContentSize();
        size.width = width;
        cell->setContentSize(size);
        ui::Helper::doLayout(cell);
        listview->pushBackCustomItem(cell);
    }
    if(listview->getItems().back()->getBottomBoundary() < 0) {
        listview->setClippingEnabled(true);
    } else {
        listview->setBounceEnabled(false);
    }
    addChild(_root);
}

void TVPListForm::show() {
    TVPMainScene::setMaskLayTouchBegain(
        [this](cocos2d::Touch * t, cocos2d::Event * e) {
            return onMaskTouchBegan(t, e);
        });
    TVPMainScene::GetInstance()->pushUIForm(this,
                                            TVPMainScene::eEnterFromBottom);
}

bool TVPListForm::onMaskTouchBegan(cocos2d::Touch *t, cocos2d::Event *) {
    Rect rc;
    rc.size = getContentSize();
    if(rc.containsPoint(convertTouchToNodeSpace(t))) {
        TVPMainScene::GetInstance()->popUIForm(this,
                                               TVPMainScene::eLeaveToBottom);
        return true;
    }
    return false;
}

TVPListForm::~TVPListForm() {
    if(_listform == this)
        _listform = nullptr;
}

int TVPDrawSceneOnce(int interval);

void TVPProcessInputEvents();

std::string TVPShowFileSelector(const std::string &title,
                                const std::string &initfilename,
                                std::string initdir, bool issave) {
    if(initdir.empty()) {
        ttstr dir = TVPGetAppPath();
        TVPGetLocalName(dir);
        initdir = dir.AsStdString();
    }
    std::string _fileSelectorResult;
    TVPFileSelectorForm *_fileSelector =
        TVPFileSelectorForm::create(initfilename, initdir, issave);
    _fileSelector->setOnClose([&](const std::string &result) {
        _fileSelectorResult = result;
        _fileSelector = nullptr;
    });
    TVPMainScene::GetInstance()->pushUIForm(_fileSelector);
    Director *director = Director::getInstance();
    while(_fileSelector) {
        TVPProcessInputEvents();
        int remain = TVPDrawSceneOnce(30); // 30 fps
        if(remain > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(remain));
        }
    }
    return _fileSelectorResult;
}

// const char *const FileName_NaviBar = "ui/NaviBar.csb";
// const char *const FileName_Body = "ui/TableView.csb";
// const char *const FileName_BottomBar = "ui/BottomBarTextInput.csb";

TVPFileSelectorForm *
TVPFileSelectorForm::create(const std::string &initfilename,
                            const std::string &initdir, bool issave) {
    auto *ret = new TVPFileSelectorForm;
    ret->autorelease();
    ret->initFromPath(initfilename, initdir, issave);
    return ret;
}

void TVPFileSelectorForm::initFromPath(const std::string &initfilename,
                                       const std::string &initdir,
                                       bool issave) {
    _isSaveMode = issave;
    this->initFromFile(Csd::createNaviBar(),
                       Csd::createTableView(Size(0, 0), 1),
                       Csd::createBottomBarTextInput());
    _input->setString(initfilename);
    ListDir(initdir); // getCurrentDir()
}

void TVPFileSelectorForm::bindFooterController(const Node *allNodes) {
    _buttonOK = static_cast<Button *>(allNodes->getChildByName("ok"));
    _buttonCancel = static_cast<Button *>(allNodes->getChildByName("cancel"));
    _input = static_cast<TextField *>(allNodes->getChildByName("input"));

    LocaleConfigManager *localeMgr = LocaleConfigManager::GetInstance();
    localeMgr->initText(_buttonOK, "ok");
    localeMgr->initText(_buttonCancel, "cancel");
    _buttonOK->addClickEventListener([this](Ref *) {
        _result = _input->getString();
        if(!_result.empty()) {
            _result = CurrentPath + "/" + _result;
            bool fileExist = FileUtils::getInstance()->isFileExist(_result);
            if(_isSaveMode) {
                if(fileExist) {
                    LocaleConfigManager *localeMgr =
                        LocaleConfigManager::GetInstance();
                    TVPMessageBoxForm::showYesNo(
                        localeMgr->GetText("ensure_to_override_file"), _result,
                        [this](int result) {
                            if(result == 0) { // yes
                                close();
                            }
                        });
                } else {
                    close();
                }
            } else { // read mode
                if(fileExist) {
                    close();
                }
            }
        }
    });
    _buttonCancel->addClickEventListener([this](Ref *) {
        _result.clear();
        close();
    });
}

void TVPFileSelectorForm::onCellClicked(int idx) {
    FileInfo info = CurrentDirList[idx];
    TVPBaseFileSelectorForm::onCellClicked(idx);
    if(info.IsDir) {
    } else {
        _input->setString(info.NameForDisplay);
    }
}

void TVPFileSelectorForm::close() {
    if(_funcOnClose)
        _funcOnClose(_result);
    TVPMainScene::GetInstance()->popUIForm(this);
}

void TVPBaseFileSelectorForm::FileItemCellImpl::init(
    const Csd::NodeBuilderFn &nodeBuilderFn, float width) {
    TTouchEventRouter::init();
    _root = nodeBuilderFn(Size(width, 48), 1);

    this->addChild(_root);
    OrigCellModelSize = _root->getContentSize();
    OrigCellModelSize.width = width;
    // _root->setContentSize(OrigCellModelSize);
    setContentSize(OrigCellModelSize);
    DirIcon = _root->getChildByName(str_diricon);
    SelectBox = _root->getChildByName<CheckBox *>(str_select);
    // SelectBox->setTouchEnabled(false);
    FileNameNode = _root->getChildByName<Text *>(str_filename);
    if(DirIcon && FileNameNode) {
        CellTextAreaSize = DirIcon->getContentSize();
        CellTextAreaSize.width =
            OrigCellModelSize.width - CellTextAreaSize.width;
        CellTextAreaSize.height = 0;
        OrigCellTextSize = FileNameNode->getContentSize();
    }
    auto *highLight = _root->getChildByName<Button *>("highlight");
    if(highLight) {
        highLight->addClickEventListener([this](Ref* ref) {
            onClicked(ref);
        });
        highLight->addTouchEventListener(
            [this](Ref *p, Widget::TouchEventType ev) {
                auto sender = dynamic_cast<Widget *>(p);
                switch(ev) {
                    case Widget::TouchEventType::BEGAN:
                        sender->scheduleOnce(
                            [this, sender](float) {
                                if(sender->isHighlighted()) {
                                    sender->scheduleOnce(
                                        [this, sender](float) {
                                            _owner->onLongPress();
                                            sender->setHighlighted(false);
                                            //	sender->interceptTouchEvent(TouchEventType::CANCELED,
                                            // sender, touch);
                                        },
                                        0, "delay_call");
                                }
                            },
                            1.0f, str_long_press);
                        break;
                    case Widget::TouchEventType::CANCELED:
                        sender->unschedule(str_long_press);
                        break;
                }
            });
    }
    BgOdd = _root->getChildByName("bg_odd");
    BgEven = _root->getChildByName("bg_even");
}

void TVPBaseFileSelectorForm::FileItemCellImpl::setInfo(int idx,
                                                        const FileInfo &info,
                                                        bool selected,
                                                        bool showSelect) {
    if(FileNameNode) {
        FileNameNode->ignoreContentAdaptWithSize(true);
        FileNameNode->setTextAreaSize(CellTextAreaSize);
        FileNameNode->setString(info.NameForDisplay);
        Size size(OrigCellModelSize);
        size.height +=
            FileNameNode->getContentSize().height - OrigCellTextSize.height;
        _root->setContentSize(size);
        setContentSize(size);
        FileNameNode->ignoreContentAdaptWithSize(false);
    }
    if(DirIcon)
        DirIcon->setVisible(info.IsDir && !showSelect);
    SelectBox->setVisible(showSelect);
    if(showSelect)
        SelectBox->setSelected(selected);
    ui::Helper::doLayout(_root);
    _set = true;
    if(BgOdd)
        BgOdd->setVisible((idx + 1) & 1);
    if(BgEven)
        BgEven->setVisible(idx & 1);
}

void TVPBaseFileSelectorForm::FileItemCellImpl::onClicked(cocos2d::Ref *p) const {
    auto *sender = dynamic_cast<Widget *>(p);
    if(sender->isScheduled(str_long_press)) {
        sender->unschedule(str_long_press);
        _owner->onClicked();
    }
}
