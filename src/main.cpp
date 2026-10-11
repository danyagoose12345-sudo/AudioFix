#include <Geode/Geode.hpp>
#include <Geode/modify/AudioOptionsLayer.hpp> // Хукаем именно аудио-вкладку настроек
#include <fmod.hpp> // Работаем напрямую с аудио-движком GD

using namespace geode::prelude;

// Глобальные указатели на FMOD эффекты
FMOD::DSP* lowpassFilter = nullptr;

// Функция для применения настроек звука
void applyFixSettings(float volume, float bass, int ghz, bool isAudioOn) {
    auto* engine = FMODAudioEngine::sharedEngine();
    if (!engine || !engine->m_system) return;

    FMOD::System* sys = engine->m_system;
    FMOD::ChannelGroup* masterGroup = nullptr;
    sys->getMasterChannelGroup(&masterGroup);

    if (masterGroup) {
        // 1. Включение / Выключение и Громкость (On/Off & Volume)
        masterGroup->setMute(!isAudioOn);
        masterGroup->setVolume(volume);

        // 2. Изменение частоты (Ghz / Pitch)
        // Эмулируем изменение Ghz через шаг скорости воспроизведения
        masterGroup->setPitch(static_cast<float>(ghz) / 44100.0f);

        // 3. Управление Басом (Bass) через Lowpass фильтр
        if (bass > 0.0f) {
            if (!lowpassFilter) {
                sys->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &lowpassFilter);
                masterGroup->addDSP(0, lowpassFilter);
            }
            // Чем больше баса, тем ниже частота среза (глуше и басовитее звук)
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

// 1. Создаем кастомный слой для подменю "Fix"
class AudioFixLayer : public FLAlertLayer {
public:
    static AudioFixLayer* create() {
        auto ret = new AudioFixLayer();
        if (ret && ret->init(240, 280, "GJ_square01.png", "Audio Fix")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init(float width, float height, const char* bg, const char* title) {
        if (!FLAlertLayer::init(nullptr, bg, {255, 255, 255, 255}, width, height)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        
        // Заголовок меню Fix
        auto titleLabel = CCLabelBMFont::create(title, "goldFont.fnt");
        titleLabel->setPosition({winSize.width / 2, winSize.height / 2 + height / 2 - 20});
        titleLabel->setScale(0.8f);
        m_mainLayer->addChild(titleLabel);

        // Кнопка Закрыть (X)
        auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(AudioFixLayer::onClose));
        
        auto menu = CCMenu::create();
        menu->addChild(closeBtn);
        menu->setPosition({winSize.width / 2 + width / 2 - 15, winSize.height / 2 + height / 2 - 15});
        m_mainLayer->addChild(menu);

        // --- ДОБАВЛЕНИЕ ЛОГИКИ ИНТЕРФЕЙСА (Volume, Bass, Ghz, On/Off) ---
        // Для создания ползунков и чекбоксов в стиле GD 2.2 используется встроенный Geode UI.
        // Ниже добавляем текстовые метки как пример структуры:
        
        createLabel("Volume", {winSize.width / 2 - 60, winSize.height / 2 + 40});
        createLabel("Bass", {winSize.width / 2 - 60, winSize.height / 2 + 10});
        createLabel("Ghz", {winSize.width / 2 - 60, winSize.height / 2 - 20});
        createLabel("Audio State", {winSize.width / 2 - 60, winSize.height / 2 - 50});

        // Пример: Кнопки-переключатели On Audio / Off Audio
        auto toggleMenu = CCMenu::create();
        toggleMenu->setPosition({winSize.width / 2 + 50, winSize.height / 2 - 50});
        
        auto onSpr = ButtonSprite::create("On", "bigFont.fnt", "GJ_button_01.png", 0.5f);
        auto onBtn = CCMenuItemSpriteExtra::create(onSpr, this, menu_selector(AudioFixLayer::onAudioOn));
        toggleMenu->addChild(onBtn);
        m_mainLayer->addChild(toggleMenu);

        return true;
    }

    void createLabel(const char* text, CCPoint pos) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setPosition(pos);
        label->setScale(0.4f);
        m_mainLayer->addChild(label);
    }

    void onAudioOn(CCObject*) {
        // Пример вызова функции с кастомными параметрами: Громкость 100%, Бас 5.0, Частота 44100, Включено
        applyFixSettings(1.0f, 5.0f, 44100, true);
    }

    void onClose(CCObject*) {
        setKeypadEnabled(false);
        removeFromParentAndCleanup(true);
    }
};

// 2. Хукаем оригинальное меню аудио-настроек Geometry Dash
class $($modify(MyAudioOptionsLayer, AudioOptionsLayer)) {
    bool init() {
        if (!AudioOptionsLayer::init()) return false;

        // Получаем доступ к меню кнопок внутри слоя настроек аудио
        auto menu = this->getChildByID("main-menu");
        if (!menu) {
            // Если ID не найден, ищем первый попавшийся CCMenu элемент
            menu = typeinfo_cast<CCMenu*>(this->getChildren()->objectAtIndex(1));
        }

        if (menu) {
            // Создаем красивую кнопку "Fix" в стиле интерфейса Робтопа
            auto fixSprite = ButtonSprite::create("Fix", "goldFont.fnt", "GJ_button_05.png", 0.6f);
            auto fixBtn = CCMenuItemSpriteExtra::create(
                fixSprite, 
                this, 
                menu_selector(MyAudioOptionsLayer::onFixPressed)
            );
            fixBtn->setID("audio-fix-button"_spr);

            // Добавляем кнопку в меню
            menu->addChild(fixBtn);
            menu->updateLayout(); // Обновляем сетку кнопок Geode
        }

        return true;
    }

    // Обработчик нажатия на кнопку "Fix"
    void onFixPressed(CCObject*) {
        auto fixLayer = AudioFixLayer::create();
        if (fixLayer) {
            CCDirector::sharedDirector()->getRunningScene()->addChild(fixLayer, 100);
        }
    }
};
