#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>  // 添加音效头文件
#include <SFML/System.hpp>
#include <string>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <windows.h>
#include <vector>

// 游戏状态枚举
enum class GameState {
    MainMenu,   // 新增：主菜单界面
    Waiting,   // 等待玩家准备
    Countdown, // 准备倒计时
    Playing,   // 游戏中
    Paused,    // 暂停状态 ← 新增
    GameOver,  // 游戏结束，等待重新开始
    Victory    // 有玩家获胜
};

struct SoundState {
    bool wasPlaying = false;
};

bool onePlayerMode = false;  // 初始为双玩家模式
float aiSpeed = 600.0f;      // AI移动速度

// 粒子结构体
struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float lifetime = 0.0f;
    float maxLifetime = 0.0f;
    float size = 0.0f;
};

int main() {
    SetConsoleOutputCP(65001);
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "Pong");
    window.setFramerateLimit(240);

    // ========== 音效系统 ==========
    // 先加载SoundBuffer
    sf::SoundBuffer bounceBuffer;
    sf::SoundBuffer scoreBuffer;
    sf::SoundBuffer countdownBuffer;
    sf::SoundBuffer victoryBuffer;
    sf::SoundBuffer backgroundBuffer;

    // 加载音效文件
    if (!backgroundBuffer.loadFromFile("sound/background.wav")) {
        std::cout << "背景音效加载失败！" << std::endl;
        return -1;
    }
    if (!bounceBuffer.loadFromFile("sound/bounce.wav")) {
        std::cout << "碰撞音效加载失败！" << std::endl;
        return -1;
    }

    if (!scoreBuffer.loadFromFile("sound/score.wav")) {
        std::cout << "得分音效加载失败！" << std::endl;
        return -1;
    }

    if (!countdownBuffer.loadFromFile("sound/countdown.wav")) {
        std::cout << "倒计时音效加载失败！" << std::endl;
        return -1;
    }

    if (!victoryBuffer.loadFromFile("sound/victory.wav")) {
        std::cout << "胜利音效加载失败！" << std::endl;
        return -1;
    }

    std::cout << "所有音效文件加载成功！" << std::endl;

    // 创建音效对象 - 必须在构造时传入SoundBuffer
    sf::Sound backgroundSound(backgroundBuffer);
    sf::Sound bounceSound(bounceBuffer);      
    sf::Sound scoreSound(scoreBuffer);        
    sf::Sound countdownSound(countdownBuffer);
    sf::Sound victorySound(victoryBuffer);    

    // 设置合适的音量
    backgroundSound.setVolume(90.f);
    bounceSound.setVolume(70.f);
    scoreSound.setVolume(80.f);
    countdownSound.setVolume(60.f);
    victorySound.setVolume(90.f);

    // 音效状态变量（放在音效对象后面）
    SoundState countdownSoundState;
    SoundState bounceSoundState;
    SoundState scoreSoundState;
    SoundState victorySoundState;
    GameState previousState = GameState::Waiting;  // 记录暂停前的状态

    // 设置背景音乐循环播放
    backgroundSound.setLooping(true);  // 重要：让背景音乐循环播放

    // ========== 菜单图片系统 ==========
    sf::Texture menuBackgroundTexture;
    sf::Sprite menuBackground(menuBackgroundTexture);

    // ========== 菜单图片加载 ==========
    if (!menuBackgroundTexture.loadFromFile("image/background.png")) {
        std::cout << "菜单背景图片加载失败！" << std::endl;
        return -1;
    }
    // 修复：重新设置Sprite的纹理
    menuBackground.setTexture(menuBackgroundTexture, true);

    // ========== 字体系统 ==========
    sf::Font font;
    if (!font.openFromFile("font/Maltais_Learlex.ttf")) {
        std::cout << "字体加载失败！" << std::endl;
        return -1;
    }
    // 创建文本对象
    sf::Text player1ScoreText(font, "0", 48);
    player1ScoreText.setFillColor(sf::Color::White);
    player1ScoreText.setPosition({ 297.f, 20.f });

    sf::Text player2ScoreText(font, "0", 48);
    player2ScoreText.setFillColor(sf::Color::White);
    player2ScoreText.setPosition({ 492.f, 20.f });

    sf::Text separatorText(font, ":", 48);
    separatorText.setFillColor(sf::Color::White);
    separatorText.setPosition({ 398.f, 20.f });

    sf::Text stateText(font, "", 30);
    stateText.setFillColor(sf::Color::Red);
    stateText.setPosition({ 240.f, 80.f });

    // 胜利文本对象
    sf::Text victoryLine1(font, "", 36);
    victoryLine1.setFillColor(sf::Color::Red);

    sf::Text victoryLine2(font, "", 24);
    victoryLine2.setFillColor(sf::Color::Red);

    // 暂停文本对象 - 使用和stateText相同的创建方式
    sf::Text pauseText1(font, "GAME PAUSED", 36);
    pauseText1.setFillColor(sf::Color::Red);
    sf::FloatRect pauseBounds1 = pauseText1.getLocalBounds();
    pauseText1.setOrigin({ pauseBounds1.size.x / 2, pauseBounds1.size.y / 2 });
    pauseText1.setPosition({ 400.f, 275.f });

    sf::Text pauseText2(font, "Press ESC to continue", 36);
    pauseText2.setFillColor(sf::Color::Red);
    sf::FloatRect pauseBounds2 = pauseText2.getLocalBounds();
    pauseText2.setOrigin({ pauseBounds2.size.x / 2, pauseBounds2.size.y / 2 });
    pauseText2.setPosition({ 400.f, 325.f });

    // 主菜单文本对象（放在其他文本对象后面）
    sf::Text titleText(font, "PONG GAME", 60);
    titleText.setFillColor(sf::Color::Red);

    sf::Text onePlayerText(font, "1 PLAYER", 36);
    onePlayerText.setFillColor(sf::Color::White);

    sf::Text twoPlayersText(font, "2 PLAYERS", 36);
    twoPlayersText.setFillColor(sf::Color::White);

    // 主菜单文本位置设置
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({ titleBounds.size.x / 2, titleBounds.size.y / 2 });
    titleText.setPosition({ 400.f, 150.f });

    sf::FloatRect onePlayerBounds = onePlayerText.getLocalBounds();
    onePlayerText.setOrigin({ onePlayerBounds.size.x / 2, onePlayerBounds.size.y / 2 });
    onePlayerText.setPosition({ 400.f, 300.f });

    sf::FloatRect twoPlayersBounds = twoPlayersText.getLocalBounds();
    twoPlayersText.setOrigin({ twoPlayersBounds.size.x / 2, twoPlayersBounds.size.y / 2 });
    twoPlayersText.setPosition({ 400.f, 370.f });

    // 按钮选择状态
    bool onePlayerSelected = true;
    // ========================================

    // 创建对象...
    // 半透明覆盖层
    sf::RectangleShape overlay;
    overlay.setSize({ 800.f, 600.f });
    overlay.setFillColor(sf::Color(80, 0, 0, 200)); 
    overlay.setPosition({ 0.f, 0.f });

    // 球拍和小球图片系统
    sf::Texture leftpaddleTexture;
    sf::Texture rightpaddleTexture;
    sf::Texture ballTexture;

    // 加载图片
    if (!leftpaddleTexture.loadFromFile("image/leftpaddle.jpg")) {
        std::cout << "左球拍图片加载失败！" << std::endl;
        return -1;
    }
    if (!rightpaddleTexture.loadFromFile("image/rightpaddle.jpg")) {
        std::cout << "右球拍图片加载失败！" << std::endl;
        return -1;
    }
    if (!ballTexture.loadFromFile("image/ball.jpg")) {
        std::cout << "小球图片加载失败！" << std::endl;
        return -1;
    }

    // 创建 Sprite
    sf::Sprite leftPaddle(leftpaddleTexture);
    sf::Sprite rightPaddle(rightpaddleTexture);
    sf::Sprite ball(ballTexture);

    // 球拍目标尺寸：20x100
    sf::Vector2f paddleTargetSize(25.f, 120.f);
    sf::Vector2u paddleTextureSize = leftpaddleTexture.getSize();

    float paddleScaleX = paddleTargetSize.x / paddleTextureSize.x;
    float paddleScaleY = paddleTargetSize.y / paddleTextureSize.y;

    leftPaddle.setScale({ paddleScaleX, paddleScaleY });
    rightPaddle.setScale({ paddleScaleX, paddleScaleY });

    // 小球目标尺寸：10x10 
    sf::Vector2f ballTargetSize(25.f, 25.f);
    sf::Vector2u ballTextureSize = ballTexture.getSize();

    float ballScaleX = ballTargetSize.x / ballTextureSize.x;
    float ballScaleY = ballTargetSize.y / ballTextureSize.y;

    ball.setScale({ ballScaleX, ballScaleY });

    // 设置初始位置
    leftPaddle.setPosition({ 50.f, 250.f });
    rightPaddle.setPosition({ 730.f, 250.f });
    ball.setPosition({ 390.f, 295.f });

    // 球拍初始位置（用于重置）
    sf::Vector2f leftPaddleStartPos(50.f, 250.f);
    sf::Vector2f rightPaddleStartPos(730.f, 250.f);

    // 改为基于时间的速度（像素/秒）
    sf::Vector2f ballVelocity(500.0f, 300.0f);
    float paddleSpeed = 700.0f;

    // 游戏状态和积分
    GameState gameState = GameState::MainMenu;
    int player1Score = 0;
    int player2Score = 0;

    // 玩家准备状态
    bool player1Ready = false;
    bool player2Ready = false;

    // 倒计时相关
    float countdownTimer = 0.0f;
    const float COUNTDOWN_DURATION = 2.0f;

    // 闪烁计时器
    float blinkTimer = 0.0f;

    // 帧计时器
    sf::Clock frameClock;

    // 质量定义
    const float paddleMass = 400.f; // 球拍质量
    const float ballMass = 314.f;   // 球质量

    // ========== 粒子系统 ==========
    std::vector<Particle> particles;
    const int PARTICLE_COUNT = 75;      // 每次爆炸的粒子数量
    const float PARTICLE_LIFETIME = 1.0f; // 粒子存活时间（秒）
    const float PARTICLE_SPEED = 300.0f;  // 粒子初始速度

    // ========== 爆炸颜色 - 恐怖血液风格 ==========
    sf::Color explosionColors[] = {
        sf::Color(120, 0, 0),       // 暗血红色
        sf::Color(160, 0, 0),       // 中等血红色
        sf::Color(200, 0, 0),       // 鲜红色
        sf::Color(140, 20, 20),     // 带棕色的血红色
        sf::Color(100, 0, 0),       // 近乎黑色的深红
        sf::Color(180, 30, 30),     // 亮血红色
        sf::Color(220, 50, 50),     // 动脉血红色
        sf::Color(130, 10, 10)      // 凝固血液色
    };
    const int COLOR_COUNT = 8;

    float aiDecisionTimer = 0.0f;
    const float AI_DECISION_INTERVAL = 0.25f;
    float aiCurrentDirection1 = 0.0f;
    float aiCurrentDirection2 = 0.0f;

    while (window.isOpen()) {
        float deltaTime = frameClock.restart().asSeconds();

        // 事件处理
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            // ESC键暂停功能
            if (event->is<sf::Event::KeyPressed>()) {
                auto keyEvent = event->getIf<sf::Event::KeyPressed>();
                if (keyEvent && keyEvent->code == sf::Keyboard::Key::Escape) {
                    // 暂停时记录音效状态
                    if (gameState == GameState::Playing || gameState == GameState::Countdown) {
                        previousState = gameState;
                        gameState = GameState::Paused;

                        // 记录哪些音效正在播放
                        countdownSoundState.wasPlaying = (countdownSound.getStatus() == sf::SoundSource::Status::Playing);
                        bounceSoundState.wasPlaying = (bounceSound.getStatus() == sf::SoundSource::Status::Playing);
                        scoreSoundState.wasPlaying = (scoreSound.getStatus() == sf::SoundSource::Status::Playing);
                        victorySoundState.wasPlaying = (victorySound.getStatus() == sf::SoundSource::Status::Playing);

                        // 停止所有音效
                        countdownSound.stop();
                        bounceSound.stop();
                        scoreSound.stop();
                        victorySound.stop();

                        std::cout << "游戏暂停" << std::endl;
                    }
                    // 恢复时重新播放音效
                    else if (gameState == GameState::Paused) {
                        gameState = previousState;

                        // 重新播放之前正在播放的音效
                        if (countdownSoundState.wasPlaying) {
                            countdownSound.play();
                        }
                        if (bounceSoundState.wasPlaying) {
                            bounceSound.play();
                        }
                        if (scoreSoundState.wasPlaying) {
                            scoreSound.play();
                        }
                        if (victorySoundState.wasPlaying) {
                            victorySound.play();
                        }

                        std::cout << "游戏继续" << std::endl;
                    }
                }
            }
        }
        // 更新闪烁计时器
        blinkTimer += deltaTime;

        // ========== 更新粒子系统 ==========
        for (auto it = particles.begin(); it != particles.end();) {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0) {
                it = particles.erase(it);
            }
            else {
                // 更新粒子位置
                it->position += it->velocity * deltaTime;
                // 添加重力效果
                it->velocity.y += 500.0f * deltaTime;
                ++it;
            }
        }

        // ========== 更新文本内容 ==========
        player1ScoreText.setString(std::to_string(player1Score));
        player2ScoreText.setString(std::to_string(player2Score));

        if (gameState == GameState::MainMenu) {
            // 主菜单状态
            if (backgroundSound.getStatus() != sf::SoundSource::Status::Playing) {
                backgroundSound.play();
            }
            // 上下键选择
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                onePlayerSelected = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                onePlayerSelected = false;
            }

            // 回车键确认选择
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) {
                if (onePlayerSelected) {
                    std::cout << "选择单玩家模式" << std::endl;
                    onePlayerMode = true;
                    backgroundSound.stop();
                    gameState = GameState::Waiting;
                }
                else {
                    std::cout << "选择双玩家模式" << std::endl;
                    onePlayerMode = false;
                    backgroundSound.stop();
                    gameState = GameState::Waiting;
                }
            }

            // 更新按钮颜色（选中的高亮）
            if (onePlayerSelected) {
                onePlayerText.setFillColor(sf::Color(120, 0, 0));
                twoPlayersText.setFillColor(sf::Color::White);
            }
            else {
                onePlayerText.setFillColor(sf::Color::White);
                twoPlayersText.setFillColor(sf::Color(120, 0, 0));
            }

        }

        else if (gameState == GameState::Waiting) {
            if (onePlayerMode) {
                stateText.setString("Press WASD Keys to Ready");
                stateText.setPosition({ 275.f, 80.f });
            }
            
            else { 
                stateText.setString("Press WASD or Arrow Keys to Ready"); 
            }
        }
        else if (gameState == GameState::Countdown) {
            stateText.setString("Starting: " + std::to_string(static_cast<int>(countdownTimer + 0.5f)));
            stateText.setPosition({ 355.f, 80.f });
        }
        else if (gameState == GameState::Playing) {
            stateText.setString("Playing");
            stateText.setPosition({ 370.f, 80.f });
        }
        else if (gameState == GameState::GameOver) {
            stateText.setString("Press any move key to continue");
            stateText.setPosition({ 260.f, 80.f });
        }
        else if (gameState == GameState::Victory) {
            // Victory状态的文本在得分时设置
        }

        // 状态文本闪烁效果
        if (gameState == GameState::Waiting || gameState == GameState::GameOver || gameState == GameState::Victory) {
            if (static_cast<int>(blinkTimer * 2) % 2 == 0) {
                stateText.setFillColor(sf::Color::Red);
                victoryLine1.setFillColor(sf::Color::Red);
                victoryLine2.setFillColor(sf::Color::Red);
            }
            else {
                stateText.setFillColor(sf::Color::Transparent);
                victoryLine1.setFillColor(sf::Color::Transparent);
                victoryLine2.setFillColor(sf::Color::Transparent);
            }
        }
        else {
            stateText.setFillColor(sf::Color::Red);
        }
        // =======================================

        // 根据游戏状态处理不同的逻辑
        if (gameState == GameState::Waiting) {
            // 等待玩家准备状态

            // 玩家1准备（按任意移动键：W、S、A、D）
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                player1Ready = true;
            }

            // 玩家2准备（按任意移动键：上、下、左、右）
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                player2Ready = true;
            }

            // 两个玩家都准备好，开始倒计时
            if (player1Ready && player2Ready && !onePlayerMode || player1Ready && onePlayerMode) {
                countdownSound.play();  // 播放倒计时音效
                gameState = GameState::Countdown;
                countdownTimer = COUNTDOWN_DURATION;
                printf("游戏将在 %.1f 秒后开始...\n", countdownTimer);
            }

        }
        else if (gameState == GameState::Countdown) {
            // 倒计时状态
            countdownTimer -= deltaTime;

            // 播放倒计时音效（只在开始时播放一次）
            static bool soundPlayed = false;
            if (!soundPlayed) {
                countdownSound.play();
                soundPlayed = true;
                std::cout << "播放倒计时音效" << std::endl;
            }

            // 更新倒计时显示
            int remainingSeconds = static_cast<int>(countdownTimer + 0.5f);
            stateText.setString("Starting: " + std::to_string(remainingSeconds));

            // 倒计时结束，开始游戏
            if (countdownTimer <= 0.0f) {
                gameState = GameState::Playing;
                soundPlayed = false;  // 重置为下次使用
                ball.setPosition({ 395.f, 295.f });

                // 随机角度（0 到 2π）
                float angle = (std::rand() % 628) / 100.0f;

                while (std::abs(std::cos(angle)) < 0.3f || std::abs(std::cos(angle)) > 0.9f) {
                    angle = (std::rand() % 628) / 100.0f;
                }

                float speed = 500.0f;
                ballVelocity.x = std::cos(angle) * speed;
                ballVelocity.y = std::sin(angle) * speed;

                printf("游戏开始！\n");
            }
        }
        else if (gameState == GameState::Playing) {
            // ========== 基于时间的运动系统 ==========

            // 球拍移动（基于时间）
            // 玩家1控制 - 左球拍
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                leftPaddle.move({ 0.f, -paddleSpeed * deltaTime });
                if (leftPaddle.getPosition().y < 0) {
                    leftPaddle.setPosition({ leftPaddle.getPosition().x, 0.f });
                }
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                leftPaddle.move({ 0.f, paddleSpeed * deltaTime });
                if (leftPaddle.getPosition().y + leftPaddle.getGlobalBounds().size.y > 600) {
                    leftPaddle.setPosition({ leftPaddle.getPosition().x, 600.f - leftPaddle.getGlobalBounds().size.y });
                }
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                leftPaddle.move({ -paddleSpeed * deltaTime, 0.f });
                if (leftPaddle.getPosition().x < 0) {
                    leftPaddle.setPosition({ 0.f, leftPaddle.getPosition().y });
                }
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                leftPaddle.move({ paddleSpeed * deltaTime, 0.f });
                if (leftPaddle.getPosition().x + leftPaddle.getGlobalBounds().size.x > 400) {
                    leftPaddle.setPosition({ 400.f - leftPaddle.getGlobalBounds().size.x, leftPaddle.getPosition().y });
                }
            }
            if (onePlayerMode) {
                // ========== 每0.25秒检测的AI控制系统 ==========

                aiDecisionTimer -= deltaTime;

                // 每0.25秒做一次决策
                if (aiDecisionTimer <= 0.0f) {
                    float ballCenterY = ball.getPosition().y + ball.getGlobalBounds().size.y / 2;
                    float paddleCenterY = rightPaddle.getPosition().y + rightPaddle.getGlobalBounds().size.y / 2;

                    // 计算目标方向
                    float distance = ballCenterY - paddleCenterY;

                    // 智能速度调节：距离越远，移动越快
                    if (std::abs(distance) > 50.0f) {
                        aiCurrentDirection1 = (distance > 0) ? 1.0f : -1.0f;  
                    }
                    else  {
                        aiCurrentDirection1 = (distance > 0) ? 0.6f : -0.6f;  
                    }
                    if (ball.getPosition().x > 400.f ) {
						aiCurrentDirection2 = (ballVelocity.x > 0) ? -1.0f : 0.5f;
                    }
                    else {
                        aiCurrentDirection2 = (ballVelocity.x > 0) ? -0.6f : 0.3f;
                    }
                    if (rightPaddle.getPosition().x < ball.getPosition().x) {
                        aiCurrentDirection2 = 1.5f;
                    }
                    aiDecisionTimer = AI_DECISION_INTERVAL; // 重置计时器
                    
                }

                // 持续应用移动（保持平滑）
                rightPaddle.move({ 0.f, aiCurrentDirection1 * aiSpeed * deltaTime });

                rightPaddle.move({ aiCurrentDirection2 * aiSpeed * deltaTime, 0.f });

                // AI边界检测（保持不变）
                if (rightPaddle.getPosition().y < 0) {
                    rightPaddle.setPosition({ rightPaddle.getPosition().x, 0.f });
                    aiCurrentDirection1 = 0.0f; // 碰到边界时停止
                }
                if (rightPaddle.getPosition().y + rightPaddle.getGlobalBounds().size.y > 600) {
                    rightPaddle.setPosition({ rightPaddle.getPosition().x, 600.f - rightPaddle.getGlobalBounds().size.y });
                    aiCurrentDirection1 = 0.0f; // 碰到边界时停止
                }
                if (rightPaddle.getPosition().x < 400) {
                    rightPaddle.setPosition({ 400.f, rightPaddle.getPosition().y });
                }
                if (rightPaddle.getPosition().x + rightPaddle.getGlobalBounds().size.x > 800) {
                    rightPaddle.setPosition({ 800.f - rightPaddle.getGlobalBounds().size.x, rightPaddle.getPosition().y });
                }
            }
            else {
                // 玩家2控制 - 右球拍
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
                    rightPaddle.move({ 0.f, -paddleSpeed * deltaTime });
                    if (rightPaddle.getPosition().y < 0) {
                        rightPaddle.setPosition({ rightPaddle.getPosition().x, 0.f });
                    }
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
                    rightPaddle.move({ 0.f, paddleSpeed * deltaTime });
                    if (rightPaddle.getPosition().y + rightPaddle.getGlobalBounds().size.y > 600) {
                        rightPaddle.setPosition({ rightPaddle.getPosition().x, 600.f - rightPaddle.getGlobalBounds().size.y });
                    }
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                    rightPaddle.move({ -paddleSpeed * deltaTime, 0.f });
                    if (rightPaddle.getPosition().x < 400) {
                        rightPaddle.setPosition({ 400.f, rightPaddle.getPosition().y });
                    }
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                    rightPaddle.move({ paddleSpeed * deltaTime, 0.f });
                    if (rightPaddle.getPosition().x + rightPaddle.getGlobalBounds().size.x > 800) {
                        rightPaddle.setPosition({ 800.f - rightPaddle.getGlobalBounds().size.x, rightPaddle.getPosition().y });
                    }
                }
            }

            // 球移动（基于时间）
            ball.move(ballVelocity * deltaTime);

            // 碰撞检测
            sf::FloatRect ballBounds = ball.getGlobalBounds();

            // 上下边界碰撞

            if (ballBounds.position.y <= 0) {
                bounceSound.play();  // 播放碰撞音效
                ballVelocity.y = std::abs(ballVelocity.y) * 0.8f;  // 确保向下
                ballVelocity.x *= 0.9f;
                // 位置修正
                ball.setPosition({ ball.getPosition().x, 0.f });
                ballBounds = ball.getGlobalBounds();
            }
            else if (ballBounds.position.y + ballBounds.size.y >= 600) {
                bounceSound.play();  // 播放碰撞音效
                ballVelocity.y = -std::abs(ballVelocity.y) * 0.8f;; // 确保向上
                ballVelocity.x *= 0.9f;
                // 位置修正
                ball.setPosition({ ball.getPosition().x, 600.f - ballBounds.size.y });
                ballBounds = ball.getGlobalBounds();
            }

            // 左右边界 - 得分 + 粒子爆炸
            if (ballBounds.position.x <= 0) {
                scoreSound.play();  // 播放得分音效
                // 创建爆炸粒子
                sf::Vector2f explosionPos = ball.getPosition();
                for (int i = 0; i < PARTICLE_COUNT; ++i) {
                    Particle p;
                    p.position = explosionPos;
                    // 随机方向
                    float angle = (std::rand() % 628) / 100.0f; // 0-2π
                    float speed = (std::rand() % 100) / 100.0f * PARTICLE_SPEED + 100.0f;
                    p.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
                    p.color = explosionColors[std::rand() % COLOR_COUNT];
                    p.lifetime = p.maxLifetime = PARTICLE_LIFETIME * (0.5f + (std::rand() % 100) / 200.0f);
                    p.size = static_cast<float>(std::rand() % 5 + 2);
                    particles.push_back(p);
                }

                player2Score++;

                // 添加胜利条件判断
                if (player2Score >= 9) {
                    gameState = GameState::Victory;
                    if (onePlayerMode) {
                        victoryLine1.setString("Computer Wins!");
                    }
                    else {
                        victoryLine1.setString("Player 2 Wins!");
                    }
                    victoryLine2.setString("Press any move key to continue");
                    // 分别居中每一行
                    sf::FloatRect bounds1 = victoryLine1.getLocalBounds();
                    victoryLine1.setOrigin({ bounds1.size.x / 2, bounds1.size.y / 2 });
                    victoryLine1.setPosition({ 400.f, 180.f });

                    sf::FloatRect bounds2 = victoryLine2.getLocalBounds();
                    victoryLine2.setOrigin({ bounds2.size.x / 2, bounds2.size.y / 2 });
                    victoryLine2.setPosition({ 400.f, 230.f });

                    victorySound.play(); // 播放胜利音效
                }
                else {
                    gameState = GameState::GameOver;
                }

                leftPaddle.setPosition(leftPaddleStartPos);
                rightPaddle.setPosition(rightPaddleStartPos);
                printf("玩家2得分! 当前比分: %d - %d\n", player1Score, player2Score);
            }
            else if (ballBounds.position.x + ballBounds.size.x >= 800) {
                scoreSound.play();  // 播放得分音效
                // 创建爆炸粒子
                sf::Vector2f explosionPos = ball.getPosition();
                for (int i = 0; i < PARTICLE_COUNT; ++i) {
                    Particle p;
                    p.position = explosionPos;
                    // 随机方向
                    float angle = (std::rand() % 628) / 100.0f; // 0-2π
                    float speed = (std::rand() % 100) / 100.0f * PARTICLE_SPEED + 100.0f;
                    p.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
                    p.color = explosionColors[std::rand() % COLOR_COUNT];
                    p.lifetime = p.maxLifetime = PARTICLE_LIFETIME * (0.5f + (std::rand() % 100) / 200.0f);
                    p.size = static_cast<float>(std::rand() % 5 + 2);
                    particles.push_back(p);
                }

                player1Score++;

                // 添加胜利条件判断
                if (player1Score >= 9) {
                    gameState = GameState::Victory;
                    victoryLine1.setString("Player 1 Wins!");
                    victoryLine2.setString("Press any move key to continue");

                    // 分别居中每一行
                    sf::FloatRect bounds1 = victoryLine1.getLocalBounds();
                    victoryLine1.setOrigin({ bounds1.size.x / 2, bounds1.size.y / 2 });
                    victoryLine1.setPosition({ 400.f, 180.f });

                    sf::FloatRect bounds2 = victoryLine2.getLocalBounds();
                    victoryLine2.setOrigin({ bounds2.size.x / 2, bounds2.size.y / 2 });
                    victoryLine2.setPosition({ 400.f, 230.f });

                    victorySound.play(); // 播放胜利音效
                }
                else {
                    gameState = GameState::GameOver;
                }

                leftPaddle.setPosition(leftPaddleStartPos);
                rightPaddle.setPosition(rightPaddleStartPos);
                printf("玩家1得分! 当前比分: %d - %d\n", player1Score, player2Score);
            }

            // 球拍碰撞检测 - 使用动量定理和位置相关速度变化
            auto leftIntersection = leftPaddle.getGlobalBounds().findIntersection(ballBounds);
            if (leftIntersection.has_value()) {
                bounceSound.play();  // 播放碰撞音效
                float totalMass = paddleMass + ballMass;

                // X方向：动量定理
                float leftPaddleVelocityX = 0.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
                    leftPaddleVelocityX = -paddleSpeed;
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                    leftPaddleVelocityX = paddleSpeed;

                ballVelocity.x = (ballMass - paddleMass) / totalMass * ballVelocity.x +
                    (2 * paddleMass) / totalMass * leftPaddleVelocityX;

                // Y方向：基于击中位置的速度变化（合并后的公式）
                float ballCenterY = ball.getPosition().y + ball.getGlobalBounds().size.x / 2;
                float hitRatio = (ballCenterY - leftPaddle.getPosition().y) / leftPaddle.getGlobalBounds().size.y;
                hitRatio = std::clamp(hitRatio, 0.0f, 1.0f);
                float hitPosition = hitRatio - 0.5f;

                // 统一的速度变化系数公式
                float speedChangeFactor = 4.0f * std::abs(hitPosition) - 1.0f;
                float baseSpeedChange = std::abs(ballVelocity.y) * 0.4f;

                // 应用速度变化（保持原方向）
                if (ballVelocity.y >= 0) {
                    ballVelocity.y += speedChangeFactor * baseSpeedChange;
                }
                else {
                    ballVelocity.y -= speedChangeFactor * baseSpeedChange;
                }

                // 速度限制
                float minYSpeed = 100.0f;
                float maxYSpeed = 600.0f;
                if (std::abs(ballVelocity.y) > maxYSpeed) {
                    ballVelocity.y = (ballVelocity.y > 0) ? maxYSpeed : -maxYSpeed;
                }
                if (std::abs(ballVelocity.y) < minYSpeed) {
                    ballVelocity.y = (ballVelocity.y > 0) ? minYSpeed : -minYSpeed;
                }

                // 确保球向右运动并修正位置
                ballVelocity.x = std::abs(ballVelocity.x);
                ball.setPosition({
                    leftPaddle.getPosition().x + leftPaddle.getGlobalBounds().size.x + 1.f,
                    ball.getPosition().y
                    });
                ballBounds = ball.getGlobalBounds();
            }

            auto rightIntersection = rightPaddle.getGlobalBounds().findIntersection(ballBounds);
            if (rightIntersection.has_value()) {
                bounceSound.play();  // 播放碰撞音效
                float totalMass = paddleMass + ballMass;

                // X方向：动量定理
                float rightPaddleVelocityX = 0.f;
                if (onePlayerMode) {
                    // 单人模式：AI控制的球拍X速度 = AI方向 * AI速度
                    rightPaddleVelocityX = aiCurrentDirection2 * aiSpeed;
                }
                else {
                    // 双人模式：玩家控制
                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
                        rightPaddleVelocityX = -paddleSpeed;
                    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
                        rightPaddleVelocityX = paddleSpeed;
                }

                ballVelocity.x = (ballMass - paddleMass) / totalMass * ballVelocity.x +
                    (2 * paddleMass) / totalMass * rightPaddleVelocityX;

                // Y方向：基于击中位置的速度变化（合并后的公式）
                float ballCenterY = ball.getPosition().y + ball.getGlobalBounds().size.x / 2;
                float hitRatio = (ballCenterY - rightPaddle.getPosition().y) / rightPaddle.getGlobalBounds().size.y;
                hitRatio = std::clamp(hitRatio, 0.0f, 1.0f);
                float hitPosition = hitRatio - 0.5f;

                // 统一的速度变化系数公式
                float speedChangeFactor = 4.0f * std::abs(hitPosition) - 1.0f;
                float baseSpeedChange = std::abs(ballVelocity.y) * 0.4f;

                // 应用速度变化（保持原方向）
                if (ballVelocity.y >= 0) {
                    ballVelocity.y += speedChangeFactor * baseSpeedChange;
                }
                else {
                    ballVelocity.y -= speedChangeFactor * baseSpeedChange;
                }

                // 速度限制
                float minYSpeed = 100.0f;
                float maxYSpeed = 600.0f;
                if (std::abs(ballVelocity.y) > maxYSpeed) {
                    ballVelocity.y = (ballVelocity.y > 0) ? maxYSpeed : -maxYSpeed;
                }
                if (std::abs(ballVelocity.y) < minYSpeed) {
                    ballVelocity.y = (ballVelocity.y > 0) ? minYSpeed : -minYSpeed;
                }

                // 确保球向左运动并修正位置
                ballVelocity.x = -std::abs(ballVelocity.x);
                ball.setPosition({
                    rightPaddle.getPosition().x - ball.getGlobalBounds().size.x / 2 * 2 - 1.f,
                    ball.getPosition().y
                    });
                ballBounds = ball.getGlobalBounds();
            }

        }
        else if (gameState == GameState::Paused) {
                if (static_cast<int>(blinkTimer * 2) % 2 == 0) {
                    pauseText2.setFillColor(sf::Color::Red);
                }
                else {
                    pauseText2.setFillColor(sf::Color::Transparent);
                }
        }
        else if (gameState == GameState::GameOver) {
            // 游戏结束状态
            static bool soundPlayed = false;
            soundPlayed = false;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                player1Ready = true;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                player2Ready = true;
            }

            if (player1Ready && player2Ready && !onePlayerMode || player1Ready && onePlayerMode) {
                gameState = GameState::Countdown;
                countdownTimer = COUNTDOWN_DURATION;
                player1Ready = false;
                player2Ready = false;
                printf("游戏将在 %.1f 秒后继续...\n", countdownTimer);
            }
        }
        else if (gameState == GameState::Victory) {
            // 胜利状态 - 与GameOver状态处理一致
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                player1Ready = true;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                player2Ready = true;
            }

            if (player1Ready && player2Ready) {
                // 重置游戏
                player1Score = 0;
                player2Score = 0;
                gameState = GameState::MainMenu;
                player1Ready = false;
                player2Ready = false;

                // 更新分数显示
                player1ScoreText.setString("0");
                player2ScoreText.setString("0");
                stateText.setString("Press WASD or Arrow Keys to Ready");
                stateText.setPosition({ 240.f, 80.f });

                printf("新游戏开始！\n");
            }
        }

        // 渲染
        window.clear(sf::Color::Black);

        // ========== 绘制粒子 ==========
        for (const auto& particle : particles) {
            float alpha = particle.lifetime / particle.maxLifetime; // 透明度衰减
            sf::CircleShape particleShape(particle.size);
            // 修复：使用标准的 unsigned char 替代 sf::Uint8
            particleShape.setFillColor(sf::Color(
                particle.color.r,
                particle.color.g,
                particle.color.b,
                static_cast<unsigned char>(alpha * 255)
            ));
            particleShape.setPosition(particle.position);
            window.draw(particleShape);
        }

        // ========== 绘制游戏对象 ==========
        // 只在游戏相关状态显示游戏对象
        if (gameState == GameState::Playing || gameState == GameState::Paused ||
            gameState == GameState::Waiting || gameState == GameState::Countdown ||
            gameState == GameState::GameOver || gameState == GameState::Victory) {

            if (gameState == GameState::Playing || gameState == GameState::Paused) {
                window.draw(leftPaddle);
                window.draw(rightPaddle);
                window.draw(ball);
            }
            else {
                ball.setPosition({ 395.f, 295.f });
                window.draw(leftPaddle);
                window.draw(rightPaddle);
                window.draw(ball);
            }
        }

        // ========== 绘制文本 ==========
        if (gameState == GameState::MainMenu) {
            window.draw(menuBackground);
            // 主菜单时只显示菜单文本，不显示游戏相关文本
            window.draw(titleText);
            window.draw(onePlayerText);
            window.draw(twoPlayersText);
        }
        else {
            if (gameState == GameState::Paused) {
                window.draw(overlay);
                window.draw(pauseText1);
                window.draw(pauseText2);
            }
            window.draw(player1ScoreText);
            window.draw(separatorText);
            window.draw(player2ScoreText);

            if (gameState == GameState::Victory) {
                window.draw(victoryLine1);
                window.draw(victoryLine2);
            }
            else {
                window.draw(stateText);
            }
        }
        // ==============================

        window.display();

        if (gameState == GameState::Countdown) {
            player1Ready = false;
            player2Ready = false;
        }
    }
    return 0;
}