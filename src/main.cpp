#include <Geode/Geode.hpp>
// Хукаем правильный класс аудио-настроек Geometry Dash 2.2
#include <Geode/modify/AudioSettingsSheet.hpp> 
#include <fmod.hpp> 

using namespace geode::prelude;

// Глобальный указатель на FMOD эффект
FMOD::DSP* lowpassFilter = nullptr;

// Функция для применения настроек звука
void applyFixSettings(float volume, float bass, int ghz, bool isAudioOn) {
    auto* engine = FMODAudioEngine::sharedEngine();
    if (!engine) return;

    // В Geode 3.X указатель на систему FMOD берется через m_system
    FMOD::System* sys = engine->m_system;
    if (!sys) return;

    FMOD::ChannelGroup* masterGroup = nullptr;
    sys->getMasterChannelGroup(&masterGroup);

    if (masterGroup) {
        // 1. Включение / Выключение и Громкость
        masterGroup->setMute(!isAudioOn);
        masterGroup->setVolume(volume);

        // 2. Изменение частоты (Ghz / Pitch)
        masterGroup->setPitch(static_cast<float>(ghz) / 44100.0f);

        // 3. Управление Басом через Lowpass фильтр
        if (bass > 0.0f) {
            if (!lowpassFilter) {
                sys->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &lowpassFilter);
                masterGroup->addDSP(0, lowpassFilter);
            }
            float cutoff = 22000.0f - (bass * 2000.0f); 
            if (cutoff < 500.0f) cutoff = 500.0f;
            lowpassFilter->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, cutoff);
        } else if (lowpassFilter) {
            masterGroup->removeDSP(lowpassFilter);
            lowpassFilter->release();
            lowpassFilter = nullptr;
        }
    }
}

// 1. Создаем слой для подменю "Fix" на базе FLAlertLayer
class AudioFixLayer : public FLAlertLayer {
public:
    static AudioFixLayer* create() {
        auto ret = new AudioFixLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    // В Geode у FLAlertLayer базовый init() не принимает кастомные размеры напрямую
    bool init() override {
        // Инициализируем стандартное окно alert-а
        if (!FLAlertLayer::init(150)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        // Создаем задний фон окна вручную нужного размера
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({240, 280});
        bg->setPosition(winSize / 2);
        m_mainLayer->addChild(bg);

        // Заголовок меню Fix
        auto titleLabel = CCLabelBMFont::create("Audio Fix", "goldFont.fnt");
        titleLabel->setPosition({winSize.width / 2, winSize.height / 2 + 120});
        titleLabel->setScale(0.8f);
        m_mainLayer->addChild(titleLabel);

        // Кнопка Закрыть (X)
        auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(AudioFixLayer::onClose));
        
        auto menu = CCMenu::create();
        menu->addChild(closeBtn);
        menu->setPosition({winSize.width / 2 + 105, winSize.height / 2 + 125});
        m_mainLayer->addChild(menu);

        // --- Интерфейс (Метки) ---
        createLabel("Volume", {winSize.width / 2 - 60, winSize.height / 2 + 40});
        createLabel("Bass", {winSize.width / 2 - 60, winSize.height / 2 + 10});
        createLabel("Ghz", {winSize.width / 2 - 60, winSize.height / 2 - 20});
        createLabel("Audio State", {winSize.width / 2 - 60, winSize.height / 2 - 50});

        // Кнопка-переключатель On Audio
        auto toggleMenu = CCMenu::create();
        toggleMenu->setPosition({winSize.width / 2 + 50, winSize.height / 2 - 50});
        
        auto onSpr = ButtonSprite::create("On", "bigFont.fnt", "GJ_button_01.png", 0.5f);
        auto onBtn = CCMenuItemSpriteExtra::create(onSpr, this, menu_selector(AudioFixLayer::onAudioOn));
        toggleMenu->addChild(onBtn);
        m_mainLayer->addChild(toggleMenu);

        // Обязательно для работы кнопки закрытия и кликов
        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        return true;
    }

    void createLabel(const char* text, CCPoint pos) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setPosition(pos);
        label->setScale(0.4f);
        m_mainLayer->addChild(label);
    }

    void onAudioOn(CCObject*) {
        applyFixSettings(1.0f, 5.0f, 44100, true);
    }

    void onClose(CCObject*) {
        setKeypadEnabled(false);
        removeFromParentAndCleanup(true);
    }
};

// 2. Хукаем оригинальное меню аудио-настроек Geometry Dash 2.2
class $modify(MyAudioSettingsSheet, AudioSettingsSheet) {
    bool init() {
        if (!AudioSettingsSheet::init()) return false;

        // В окне AudioSettingsSheet основное меню кнопок обычно располагается в m_mainLayer
        auto menu = typeinfo_cast<CCMenu*>(this->m_mainLayer->getChildByID("main-menu"));
        if (!menu) {
            // Запасной вариант, если ID не определен Geode
            menu = typeinfo_cast<CCMenu*>(this->m_mainLayer->getChildren()->objectAtIndex(1));
        }

        if (menu) {
            // Создаем кнопку "Fix"
            auto fixSprite = ButtonSprite::create("Fix", "goldFont.fnt", "GJ_button_05.png", 0.6f);
            auto fixBtn = CCMenuItemSpriteExtra::create(
                fixSprite, 
                this, 
                menu_selector(MyAudioSettingsSheet::onFixPressed)
            );
            fixBtn->setID("audio-fix-button"_spr);

            // Добавляем её в интерфейс Робтопа
            menu->addChild(fixBtn);
            menu->updateLayout(); 
        }

        return true;
    }

    void onFixPressed(CCObject*) {
        auto fixLayer = AudioFixLayer::create();
        if (fixLayer) {
            // Родным методом для вызова всплывающих окон является метод .show() у FLAlertLayer
            fixLayer->show();
        }
    }
};

