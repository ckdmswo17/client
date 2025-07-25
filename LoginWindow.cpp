#include "LoginWindow.h"
#include "ui_LoginWindow.h"
#include "MainWindow.h"
#include "TcpCommunicator.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTimer>
#include <QPixmap>
#include <QBuffer>
#include <QByteArray>
#include <QRegularExpression>

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui_LoginWindow)
    , m_tcpCommunicator(nullptr)
    , m_passwordErrorLabel(nullptr)
    , m_closeSignUpButton(nullptr)
    , m_closeOtpSignUpButton(nullptr)
    , m_connectionStatusLabel(nullptr)
    , m_connectionTimer(nullptr)
{
    qDebug() << "[LoginWindow] 생성자 시작";

    ui->setupUi(this);

    // 초기화 메서드 호출
    setupPasswordFields();
    setupPasswordErrorLabel();
    setupCloseButtons();
    setupConnectionStatusLabel();

    qDebug() << "[LoginWindow] UI 설정 완료";

    // TCP 통신 설정
    setupTcpCommunication();

    qDebug() << "[LoginWindow] TCP 통신 설정 완료";

    // 시그널 연결
    connectSignals();

    qDebug() << "[LoginWindow] 시그널 연결 완료";

    // 초기 페이지 설정
    ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
    ui->OTPLoginWidget->setCurrentIndex(0); // LgoinPage

    // 다이얼로그 설정
    setModal(true);
    setWindowTitle("CCTV 모니터링 시스템 - 로그인");

    // 연결 상태 확인 타이머 설정
    setupConnectionTimer();

    qDebug() << "[LoginWindow] 생성자 완료";
}

LoginWindow::~LoginWindow()
{
    qDebug() << "[LoginWindow] 소멸자 호출";

    if (m_tcpCommunicator) {
        m_tcpCommunicator->disconnectFromServer();
        m_tcpCommunicator->deleteLater();
        m_tcpCommunicator = nullptr;
    }

    if (m_connectionTimer) {
        m_connectionTimer->stop();
    }

    delete ui;

    qDebug() << "[LoginWindow] 소멸자 완료";
}

void LoginWindow::setupConnectionStatusLabel()
{
    // 연결 상태 표시 라벨 생성 (로그인 페이지 하단에)
    m_connectionStatusLabel = new QLabel(ui->LgoinPage);
    m_connectionStatusLabel->setText("서버 연결 중...");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #ff6b35;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );
    m_connectionStatusLabel->setGeometry(40, 300, 200, 15); // 로그인 버튼 아래
    m_connectionStatusLabel->show();
}

void LoginWindow::setupConnectionTimer()
{
    // 연결 상태 확인 타이머 (5초마다 확인)
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setInterval(5000); // 5초
    connect(m_connectionTimer, &QTimer::timeout, this, &LoginWindow::checkConnectionStatus);
    m_connectionTimer->start();
}

void LoginWindow::checkConnectionStatus()
{
    qDebug() << "[LoginWindow] 연결 상태 확인";

    if (m_tcpCommunicator) {
        bool isConnected = m_tcpCommunicator->isConnectedToServer();
        qDebug() << "[LoginWindow] TCP 연결 상태:" << isConnected;

        if (isConnected) {
            m_connectionStatusLabel->setText("서버 연결됨");
            m_connectionStatusLabel->setStyleSheet(
                "QLabel {"
                "    color: #28a745;"
                "    font-size: 11px;"
                "    background-color: transparent;"
                "}"
                );
        } else {
            m_connectionStatusLabel->setText("서버 연결 끊어짐 - 재연결 시도 중...");
            m_connectionStatusLabel->setStyleSheet(
                "QLabel {"
                "    color: #dc3545;"
                "    font-size: 11px;"
                "    background-color: transparent;"
                "}"
                );

            // 재연결 시도
            m_tcpCommunicator->connectToServer("192.168.0.81", 8080);
        }
    } else {
        qDebug() << "[LoginWindow] TcpCommunicator가 초기화되지 않음";
    }
}

void LoginWindow::setupPasswordFields()
{
    qDebug() << "[LoginWindow] 패스워드 필드 설정";

    // 로그인 페이지 패스워드 필드
    ui->pwLineEdit->setEchoMode(QLineEdit::Password);

    // 회원가입 페이지 패스워드 필드들
    ui->pwLineEdit_3->setEchoMode(QLineEdit::Password);
    ui->pwLineEdit_4->setEchoMode(QLineEdit::Password);

    // 비밀번호 확인 필드에 실시간 검증 연결
    connect(ui->pwLineEdit_3, &QLineEdit::textChanged, this, &LoginWindow::onPasswordChanged);
    connect(ui->pwLineEdit_4, &QLineEdit::textChanged, this, &LoginWindow::onPasswordChanged);
}

void LoginWindow::setupPasswordErrorLabel()
{
    qDebug() << "[LoginWindow] 패스워드 오류 라벨 설정";

    // 비밀번호 오류 메시지 라벨 생성
    m_passwordErrorLabel = new QLabel(ui->SignUp);
    m_passwordErrorLabel->setText("* 비밀번호가 다릅니다");
    m_passwordErrorLabel->setStyleSheet("color: red; font-size: 12px;");
    m_passwordErrorLabel->setGeometry(40, 385, 200, 20); // SUBMIT 버튼 위에 위치
    m_passwordErrorLabel->hide(); // 초기에는 숨김
}

void LoginWindow::setupCloseButtons()
{
    qDebug() << "[LoginWindow] 닫기 버튼 설정";

    // Sign Up 페이지 닫기 버튼
    m_closeSignUpButton = new QPushButton("×", ui->SignUp);
    m_closeSignUpButton->setGeometry(295, 10, 25, 25); // 오른쪽 상단 모서리
    m_closeSignUpButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #666;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e81123;"
        "    color: white;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c50e1f;"
        "}"
        );

    // OTP Sign Up 페이지 닫기 버튼
    m_closeOtpSignUpButton = new QPushButton("×", ui->OTPSignUp);
    m_closeOtpSignUpButton->setGeometry(295, 10, 25, 25); // 오른쪽 상단 모서리
    m_closeOtpSignUpButton->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: none;"
        "    color: #666;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e81123;"
        "    color: white;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c50e1f;"
        "}"
        );
}

void LoginWindow::setupTcpCommunication()
{
    qDebug() << "[LoginWindow] TCP 통신 설정 시작";

    // TcpCommunicator 인스턴스 생성
    m_tcpCommunicator = new TcpCommunicator(this);

    // TCP 시그널 연결
    connect(m_tcpCommunicator, &TcpCommunicator::connected,
            this, &LoginWindow::onTcpConnected);
    connect(m_tcpCommunicator, &TcpCommunicator::disconnected,
            this, &LoginWindow::onTcpDisconnected);
    connect(m_tcpCommunicator, &TcpCommunicator::errorOccurred,
            this, &LoginWindow::onTcpError);
    connect(m_tcpCommunicator, &TcpCommunicator::messageReceived,
            this, &LoginWindow::onTcpMessageReceived);

    // 서버 연결 시도
    qDebug() << "[LoginWindow] 서버 연결 시도: 192.168.0.81:8080";
    m_tcpCommunicator->connectToServer("192.168.0.81", 8080);

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("서버 연결 시도 중...");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #ffc107;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );

    qDebug() << "[LoginWindow] TCP 통신 설정 완료";
}

void LoginWindow::connectSignals()
{
    qDebug() << "[LoginWindow] 시그널 연결 시작";

    // 로그인 페이지 시그널 연결
    connect(ui->SubmitButton, &QPushButton::clicked,
            this, &LoginWindow::handleLogin);
    connect(ui->SignUpButton, &QPushButton::clicked,
            this, &LoginWindow::handleSignUpSwitch);

    // OTP 로그인 페이지 시그널 연결
    connect(ui->SubmitButton_2, &QPushButton::clicked,
            this, &LoginWindow::handleSubmitOtpLogin);
    connect(ui->SignUpButton_2, &QPushButton::clicked,
            this, &LoginWindow::handleSignUpSwitch);

    // 회원가입 페이지 시그널 연결
    connect(ui->SubmitButton_5, &QPushButton::clicked,
            this, &LoginWindow::handleSubmitSignUp);

    // OTP 회원가입 페이지 시그널 연결
    connect(ui->SubmitButton_6, &QPushButton::clicked,
            this, &LoginWindow::handleSubmitOtpSignUp);

    // 닫기 버튼 시그널 연결
    connect(m_closeSignUpButton, &QPushButton::clicked,
            this, &LoginWindow::handleCloseSignUp);
    connect(m_closeOtpSignUpButton, &QPushButton::clicked,
            this, &LoginWindow::handleCloseOtpSignUp);
}

void LoginWindow::handleCloseSignUp()
{
    qDebug() << "[LoginWindow] Sign Up 닫기 버튼 클릭";

    // 회원가입 창을 닫고 로그인 페이지로 돌아가기
    ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
    ui->OTPLoginWidget->setCurrentIndex(0); // LgoinPage
    clearSignUpFields();
}

void LoginWindow::handleCloseOtpSignUp()
{
    qDebug() << "[LoginWindow] OTP Sign Up 닫기 버튼 클릭";

    // OTP 회원가입 창을 닫고 일반 회원가입 페이지로 돌아가기
    ui->OTPLoginWidget_3->setCurrentIndex(0); // SignUpPage
}

void LoginWindow::onPasswordChanged()
{
    qDebug() << "[LoginWindow] 패스워드 변경됨";

    QString password = ui->pwLineEdit_3->text();
    QString confirmPassword = ui->pwLineEdit_4->text();

    // 둘 다 비어있으면 오류 메시지 숨김
    if (password.isEmpty() && confirmPassword.isEmpty()) {
        m_passwordErrorLabel->hide();
        return;
    }

    // 확인 비밀번호가 비어있으면 오류 메시지 숨김
    if (confirmPassword.isEmpty()) {
        m_passwordErrorLabel->hide();
        return;
    }

    // 비밀번호가 다르면 오류 메시지 표시
    if (password != confirmPassword) {
        m_passwordErrorLabel->show();
    } else {
        m_passwordErrorLabel->hide();
    }
}

void LoginWindow::handleLogin()
{
    qDebug() << "[LoginWindow] 로그인 버튼 클릭";

    QString id = ui->idLineEdit->text().trimmed();
    QString password = ui->pwLineEdit->text().trimmed();

    if (id.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "아이디와 패스워드를 모두 입력해주세요.");
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.\n잠시 후 다시 시도해주세요.");

        // 재연결 시도
        m_tcpCommunicator->connectToServer("192.168.0.81", 8080);
        return;
    }

    // 로그인 버튼 비활성화
    ui->SubmitButton->setEnabled(false);
    ui->SubmitButton->setText("로그인 중...");

    // 로그인 요청 전송
    sendLoginRequest(id, password);
}

void LoginWindow::handleSignUpSwitch()
{
    qDebug() << "[LoginWindow] Sign Up 전환";

    // 회원가입 페이지로 전환 (page_4)
    ui->stackedWidget->setCurrentIndex(1); // page_4 (회원가입 페이지)
    ui->OTPLoginWidget_3->setCurrentIndex(0); // SignUpPage

    // 입력 필드 초기화
    clearSignUpFields();
}

void LoginWindow::handleOtpSignupSwitch()
{
    qDebug() << "[LoginWindow] OTP Sign Up 전환";

    // OTP 회원가입 페이지로 전환
    ui->OTPLoginWidget_3->setCurrentIndex(1); // OTPSignUpPage
}

void LoginWindow::handleSubmitSignUp()
{
    qDebug() << "[LoginWindow] Sign Up 제출";

    QString id = ui->IDLabel->text().trimmed();
    QString password = ui->pwLineEdit_3->text().trimmed();
    QString confirmPassword = ui->pwLineEdit_4->text().trimmed();
    bool useOtp = ui->checkBox->isChecked();

    // 입력 유효성 검사
    if (id.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "모든 필드를 입력해주세요.");
        return;
    }

    // 비밀번호 일치 검증
    if (password != confirmPassword) {
        m_passwordErrorLabel->show();
        QMessageBox::warning(this, "비밀번호 오류", "비밀번호가 일치하지 않습니다.");
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.\n잠시 후 다시 시도해주세요.");

        // 재연결 시도
        m_tcpCommunicator->connectToServer("192.168.0.81", 8080);
        return;
    }

    // 비밀번호가 일치하면 오류 메시지 숨김
    m_passwordErrorLabel->hide();

    if (useOtp) {
        // OTP 회원가입 페이지로 전환
        handleOtpSignupSwitch();
        // 여기서 OTP QR 코드 생성 요청을 서버에 보낼 수 있음
        generateOtpQrCode(id, password);
    } else {
        // 일반 회원가입 처리
        sendSignUpRequest(id, password, false);
    }
}

void LoginWindow::handleSubmitOtpLogin()
{
    qDebug() << "[LoginWindow] OTP 로그인 제출";

    QString otpCode = ui->idLineEdit_2->text().trimmed();

    if (otpCode.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "OTP 코드를 입력해주세요.");
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        resetOtpLoginButton();
        return;
    }

    // OTP 로그인 버튼 비활성화
    ui->SubmitButton_2->setEnabled(false);
    ui->SubmitButton_2->setText("인증 중...");

    // OTP 로그인 요청 전송
    sendOtpLoginRequest(otpCode);
}

void LoginWindow::handleSubmitOtpSignUp()
{
    qDebug() << "[LoginWindow] OTP Sign Up 제출";

    QString otpCode = ui->lineEdit->text().trimmed();

    if (otpCode.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "OTP 코드를 입력해주세요.");
        return;
    }

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        return;
    }

    // OTP 회원가입 요청 전송
    sendOtpSignUpRequest(otpCode);
}

void LoginWindow::sendLoginRequest(const QString &id, const QString &password)
{
    qDebug() << "[LoginWindow] 로그인 요청 전송 - ID:" << id;

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        resetLoginButton();
        return;
    }

    // 로그인 요청 JSON 생성 (request_id: 8)
    QJsonObject loginMessage;
    loginMessage["request_id"] = 8;

    QJsonObject data;
    data["id"] = id;
    data["passwd"] = password;  // password -> passwd로 변경
    loginMessage["data"] = data;

    // 현재 로그인 정보 저장 (OTP 로그인용)
    m_currentUserId = id;
    m_currentPassword = password;

    qDebug() << "[LoginWindow] 전송할 JSON:" << QJsonDocument(loginMessage).toJson(QJsonDocument::Compact);

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(loginMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "로그인 정보 전송에 실패했습니다.");
        resetLoginButton();
    } else {
        qDebug() << "[LoginWindow] 로그인 요청 전송 성공";
    }
}

void LoginWindow::sendSignUpRequest(const QString &id, const QString &password, bool useOtp)
{
    qDebug() << "[LoginWindow] 회원가입 요청 전송 - ID:" << id << "OTP:" << useOtp;

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        return;
    }

    // 회원가입 요청 JSON 생성 (request_id: 9)
    QJsonObject signUpMessage;
    signUpMessage["request_id"] = 9;

    QJsonObject data;
    data["id"] = id;
    data["passwd"] = password;  // password -> passwd로 변경
    signUpMessage["data"] = data;

    qDebug() << "[LoginWindow] 전송할 JSON:" << QJsonDocument(signUpMessage).toJson(QJsonDocument::Compact);

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(signUpMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "회원가입 정보 전송에 실패했습니다.");
    } else {
        qDebug() << "[LoginWindow] 회원가입 요청 전송 성공";
    }
}

void LoginWindow::sendOtpLoginRequest(const QString &otpCode)
{
    qDebug() << "[LoginWindow] OTP 로그인 요청 전송";

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        resetOtpLoginButton();
        return;
    }

    // OTP 로그인 요청 JSON 생성 (request_id: 1003)
    QJsonObject otpLoginMessage;
    otpLoginMessage["request_id"] = 1003;
    otpLoginMessage["id"] = m_currentUserId;
    otpLoginMessage["otp_code"] = otpCode;

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(otpLoginMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "OTP 인증 정보 전송에 실패했습니다.");
        resetOtpLoginButton();
    }

    qDebug() << "[LoginWindow] OTP 로그인 요청 전송 - ID:" << m_currentUserId;
}

void LoginWindow::sendOtpSignUpRequest(const QString &otpCode)
{
    qDebug() << "[LoginWindow] OTP 회원가입 요청 전송";

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        return;
    }

    // OTP 회원가입 완료 요청 JSON 생성 (request_id: 1004)
    QJsonObject otpSignUpMessage;
    otpSignUpMessage["request_id"] = 1004;
    otpSignUpMessage["otp_code"] = otpCode;

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(otpSignUpMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "OTP 회원가입 정보 전송에 실패했습니다.");
    }

    qDebug() << "[LoginWindow] OTP 회원가입 요청 전송";
}

void LoginWindow::generateOtpQrCode(const QString &id, const QString &password)
{
    qDebug() << "[LoginWindow] OTP QR 코드 생성 요청";

    // TCP 연결 확인
    if (!m_tcpCommunicator || !m_tcpCommunicator->isConnectedToServer()) {
        QMessageBox::warning(this, "연결 오류", "서버에 연결되지 않았습니다.");
        return;
    }

    // OTP QR 코드 생성 요청 JSON (request_id: 1005)
    QJsonObject qrCodeMessage;
    qrCodeMessage["request_id"] = 1005;
    qrCodeMessage["id"] = id;
    qrCodeMessage["passwd"] = password;  // password -> passwd로 변경

    // 현재 회원가입 정보 저장
    m_currentUserId = id;
    m_currentPassword = password;

    // 서버로 전송
    bool success = m_tcpCommunicator->sendJsonMessage(qrCodeMessage);
    if (!success) {
        QMessageBox::warning(this, "전송 오류", "OTP QR 코드 요청에 실패했습니다.");
    }

    qDebug() << "[LoginWindow] OTP QR 코드 생성 요청 전송 - ID:" << id;
}

void LoginWindow::resetLoginButton()
{
    ui->SubmitButton->setEnabled(true);
    ui->SubmitButton->setText("SUBMIT");
}

void LoginWindow::resetOtpLoginButton()
{
    ui->SubmitButton_2->setEnabled(true);
    ui->SubmitButton_2->setText("SUBMIT");
}

void LoginWindow::clearSignUpFields()
{
    ui->IDLabel->clear();
    ui->pwLineEdit_3->clear();
    ui->pwLineEdit_4->clear();
    ui->checkBox->setChecked(false);
    m_passwordErrorLabel->hide(); // 오류 메시지도 숨김
}

void LoginWindow::clearLoginFields()
{
    ui->idLineEdit->clear();
    ui->pwLineEdit->clear();
    ui->idLineEdit_2->clear();
    ui->lineEdit->clear();
}

void LoginWindow::onTcpConnected()
{
    qDebug() << "[LoginWindow] TCP 연결 성공";

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("서버 연결됨");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #28a745;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );
}

void LoginWindow::onTcpDisconnected()
{
    qDebug() << "[LoginWindow] TCP 연결 해제";

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("서버 연결 끊어짐");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #dc3545;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );

    // 사용자에게 알림 (한 번만)
    static bool disconnectNotified = false;
    if (!disconnectNotified) {
        QMessageBox::warning(this, "연결 해제", "서버와의 연결이 끊어졌습니다.\n자동으로 재연결을 시도합니다.");
        disconnectNotified = true;

        // 5초 후 알림 플래그 리셋
        QTimer::singleShot(5000, [&]() {
            disconnectNotified = false;
        });
    }
}

void LoginWindow::onTcpError(const QString &error)
{
    qDebug() << "[LoginWindow] TCP 오류:" << error;

    // 연결 상태 업데이트
    m_connectionStatusLabel->setText("연결 오류 - 재시도 중...");
    m_connectionStatusLabel->setStyleSheet(
        "QLabel {"
        "    color: #dc3545;"
        "    font-size: 11px;"
        "    background-color: transparent;"
        "}"
        );

    // 버튼 상태 복원
    resetLoginButton();
    resetOtpLoginButton();

    // 오류 로그 출력
    qDebug() << "[LoginWindow] 상세 오류 정보:" << error;
}

void LoginWindow::onTcpMessageReceived(const QString &message)
{
    qDebug() << "[LoginWindow] TCP 메시지 수신:" << message;

    // JSON 파싱
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "[LoginWindow] JSON 파싱 오류:" << error.errorString();
        qDebug() << "[LoginWindow] 원본 메시지:" << message;
        return;
    }

    QJsonObject jsonObj = doc.object();
    int requestId = jsonObj["request_id"].toInt();

    qDebug() << "[LoginWindow] 파싱된 request_id:" << requestId;

    // 응답 처리
    switch (requestId) {
    case 19: // 로그인 응답
        qDebug() << "[LoginWindow] 로그인 응답 처리";
        handleLoginResponse(jsonObj);
        break;
    case 20: // 회원가입 응답
        qDebug() << "[LoginWindow] 회원가입 응답 처리";
        handleSignUpResponse(jsonObj);
        break;
    case 10: // OTP 로그인 응답 (필요시)
        qDebug() << "[LoginWindow] OTP 로그인 응답 처리";
        handleOtpLoginResponse(jsonObj);
        break;
    case 11: // OTP 회원가입 응답 (필요시)
        qDebug() << "[LoginWindow] OTP 회원가입 응답 처리";
        handleOtpSignUpResponse(jsonObj);
        break;
    case 12: // OTP QR 코드 응답 (필요시)
        qDebug() << "[LoginWindow] QR 코드 응답 처리";
        handleQrCodeResponse(jsonObj);
        break;
    default:
        qDebug() << "[LoginWindow] 알 수 없는 request_id:" << requestId;
        qDebug() << "[LoginWindow] 전체 JSON:" << QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
        break;
    }
}

void LoginWindow::handleLoginResponse(const QJsonObject &response)
{
    int success = response["login_success"].toInt();
    QString message = response["message"].toString();
    bool isOtpUser = response["is_otp_user"].toBool(false);

    qDebug() << "[LoginWindow] 로그인 응답 - 성공:" << success << "메시지:" << message << "OTP 사용자:" << isOtpUser;

    resetLoginButton();

    if (success != 0) {
        if (isOtpUser) {
            // OTP 사용자인 경우 OTP 로그인 페이지로 전환
            ui->OTPLoginWidget->setCurrentIndex(1); // OTPLoginPage
            ui->idLineEdit_2->setFocus();
            QMessageBox::information(this, "OTP 인증", "OTP 인증번호를 입력해주세요.");
        } else {
            // 일반 사용자인 경우 로그인 성공
            QMessageBox::information(this, "로그인 성공", "로그인에 성공했습니다.");
            emit loginSuccessful();
            accept();
        }
    } else {
        QMessageBox::warning(this, "로그인 실패", message.isEmpty() ? "로그인에 실패했습니다." : message);
        clearLoginFields();
    }
}

void LoginWindow::handleSignUpResponse(const QJsonObject &response)
{
    int success = response["sign_up_success"].toInt();
    QString message = response["message"].toString();

    qDebug() << "[LoginWindow] 회원가입 응답 - 성공:" << success << "메시지:" << message;

    if (success != 0) {
        QMessageBox::information(this, "회원가입 성공", "회원가입이 완료되었습니다.");
        // 로그인 페이지로 돌아가기
        ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
        ui->OTPLoginWidget->setCurrentIndex(0); // LgoinPage
        clearSignUpFields();
        clearLoginFields();
    } else {
        QMessageBox::warning(this, "회원가입 실패", message.isEmpty() ? "회원가입에 실패했습니다." : message);
    }
}

void LoginWindow::handleOtpLoginResponse(const QJsonObject &response)
{
    bool success = response["success"].toBool();
    QString message = response["message"].toString();

    resetOtpLoginButton();

    if (success) {
        QMessageBox::information(this, "로그인 성공", "OTP 인증이 완료되었습니다.");
        emit loginSuccessful();
        accept();
    } else {
        QMessageBox::warning(this, "OTP 인증 실패", message.isEmpty() ? "OTP 인증에 실패했습니다." : message);
        ui->idLineEdit_2->clear();
        ui->idLineEdit_2->setFocus();
    }
}

void LoginWindow::handleOtpSignUpResponse(const QJsonObject &response)
{
    bool success = response["success"].toBool();
    QString message = response["message"].toString();

    if (success) {
        QMessageBox::information(this, "회원가입 성공", "OTP 회원가입이 완료되었습니다.");
        // 로그인 페이지로 돌아가기
        ui->stackedWidget->setCurrentIndex(0); // page_3 (로그인 페이지)
        ui->OTPLoginWidget->setCurrentIndex(0); // LgoinPage
        clearSignUpFields();
        clearLoginFields();
    } else {
        QMessageBox::warning(this, "OTP 회원가입 실패", message.isEmpty() ? "OTP 회원가입에 실패했습니다." : message);
    }
}

void LoginWindow::handleQrCodeResponse(const QJsonObject &response)
{
    bool success = response["success"].toBool();
    QString message = response["message"].toString();
    QString qrCodeData = response["qr_code"].toString();

    if (success && !qrCodeData.isEmpty()) {
        // QR 코드를 UI에 표시 (여기서는 간단히 메시지로 표시)
        QMessageBox::information(this, "OTP 설정",
                                 "QR 코드가 생성되었습니다.\n"
                                 "OTP 앱으로 QR 코드를 스캔한 후 인증번호를 입력해주세요.\n\n"
                                 "QR 코드 데이터: " + qrCodeData);

        // 실제로는 QR 코드 이미지를 frame에 표시해야 함
        // TODO: QR 코드 이미지 표시 구현

    } else {
        QMessageBox::warning(this, "QR 코드 생성 실패", message.isEmpty() ? "QR 코드 생성에 실패했습니다." : message);
        // 일반 회원가입 페이지로 돌아가기
        ui->OTPLoginWidget_3->setCurrentIndex(0); // SignUpPage
    }
}
