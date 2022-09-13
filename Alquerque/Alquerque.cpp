#include "Alquerque.h"
#include "ui_Alquerque.h"

#include <QDebug>
#include <QMessageBox>
#include <QActionGroup>
#include <QSignalMapper>

Alquerque::Player state2player(Hole::State state) {
    switch (state) {
        case Hole::RedState:
            return Alquerque::RedPlayer;
        case Hole::BlueState:
            return Alquerque::BluePlayer;
        default:
            Q_UNREACHABLE();
    }
}

Alquerque::Player otherPlayer(Alquerque::Player player) {
    return (player == Alquerque::RedPlayer ?
                    Alquerque::BluePlayer : Alquerque::RedPlayer);
}

//Converte um jogador para seu Hole::state
Hole::State player2state(Alquerque::Player player) {
    return player == Alquerque::RedPlayer ? Hole::RedState : Hole::BlueState;
}

Alquerque::Alquerque(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Alquerque),
      m_player(Alquerque::RedPlayer){

    ui->setupUi(this);

    QObject::connect(ui->actionNew, SIGNAL(triggered(bool)), this, SLOT(reset()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    QObject::connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    QSignalMapper* map = new QSignalMapper(this);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            QString holeName = QString("hole%1%2").arg(row).arg(col);
            Hole* hole = this->findChild<Hole*>(holeName);
            Q_ASSERT(hole != nullptr);

            m_board[row][col] = hole;

            int id = row * 5 + col;
            map->setMapping(hole, id);
            QObject::connect(hole, SIGNAL(clicked(bool)), map, SLOT(map()));
        }
    }
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QObject::connect(map, SIGNAL(mapped(int)), this, SLOT(play(int)));
#else
    QObject::connect(map, SIGNAL(mappedInt(int)), this, SLOT(play(int)));
#endif

    // When the turn ends.
    QObject::connect(this, SIGNAL(turnEnded()), this, SLOT(endTurn()));

    this->reset();

    this->adjustSize();
    this->setFixedSize(this->size());
}

Alquerque::~Alquerque() {
    delete ui;
}

void Alquerque::play(int id) {
    Hole* hole = m_board[id / 5][id % 5];
    qDebug() << "clicked on: " << hole->objectName();
    switch(m_state){
        case 1: //selecao da peca a ser jogada
            if(holeInAvailableStartSelection(id)){
                hole->setMarked(true);
                markAvailablePlays(id);
                m_state = 2;
                m_last_played_hole_id = id;
            }
            break;
        case 2: //selecao da posicao final da jogada ou resselecionar peca
        if(id == m_last_played_hole_id){
            removeMarkings();
            hole->setMarked(false);
            m_state = 1;
        }
        else if(holeInAvailableStartSelection(id)){
                removeMarkings();
                hole->setMarked(true);
                markAvailablePlays(id);
                m_last_played_hole_id = id;
                //m_state = 2;
            }
            else if(holeInAvailableEndSelection(m_last_played_hole_id, id)){
                removeMarkings();
                int row_initial = m_last_played_hole_id/5;
                int col_initial = m_last_played_hole_id%5;
                int row_final = id/5;
                int col_final = id%5;
                bool fim_turno = false;
                m_board[row_initial][col_initial]->setState(Hole::EmptyState);

                int row_diff = row_final - row_initial;
                int col_diff = col_final - col_initial;

                if(m_board[row_initial + row_diff/2][col_initial + col_diff/2]->state() == player2state(otherPlayer(m_player))){
                    if(m_player == Alquerque::RedPlayer){
                        m_number_blue_pieces--;
                    }
                    else{
                        m_number_red_pieces--;
                    }

                    m_board[row_initial + row_diff/2][col_initial + col_diff/2]->setState(Hole::EmptyState);
                    m_board[row_final][col_final]->setState(player2state(m_player));

                    if(hasSubsequentPlay(id)){
                        m_last_played_hole_id = id;
                        m_state = 3;
                        removeMarkings();
                        m_board[id/5][id%5]->setMarked(true);
                        markAvailablePlays(id);
                        markSelectableHoles();
                    }
                    else{
                        fim_turno = true;
                    }
                }
                else{
                    m_board[row_final][col_final]->setState(player2state(m_player));
                    fim_turno = true;
                }
                if(fim_turno){
                    emit turnEnded();
                }
            }
            break;
        case 3: //verificar jogadas subsequentes apos uma jogada obrigatoria (peca toma outra peca)
            if(holeInAvailableEndSelection(m_last_played_hole_id, id)){
                removeMarkings();
                int row_initial = m_last_played_hole_id/5;
                int col_initial = m_last_played_hole_id%5;
                int row_final = id/5;
                int col_final = id%5;
                m_board[row_initial][col_initial]->setState(Hole::EmptyState);

                int row_diff = row_final - row_initial;
                int col_diff = col_final - col_initial;

                if(m_player == Alquerque::RedPlayer){
                    m_number_blue_pieces--;
                }
                else{
                    m_number_red_pieces--;
                }

                m_board[row_initial + row_diff/2][col_initial + col_diff/2]->setState(Hole::EmptyState);
                m_board[row_final][col_final]->setState(player2state(m_player));

                m_last_played_hole_id = id;
                if(hasSubsequentPlay(id)){
                    m_last_played_hole_id = id;
                    m_state = 3;
                    removeMarkings();
                    m_board[id/5][id%5]->setMarked(true);
                    markAvailablePlays(id);
                    markSelectableHoles();
                }
                else{
                    emit turnEnded();
                }
            }
            break;
        default:
            break;
    }
}

void Alquerque::endTurn() {
    m_state = 1;

    if(m_number_red_pieces == 0){
        QMessageBox::information(this, tr("Fim de jogo"), tr("Jogador Azul Venceu"));
        reset();
    }
    else if(m_number_blue_pieces == 0){
        QMessageBox::information(this, tr("Fim de jogo"), tr("Jogador Vermelho Venceu"));
        reset();
    }
    else{
        // Switch the player.
        m_player = otherPlayer(m_player);

        //verificacao de fim de jogo por falta de movimento
        auto availablePlays = getObligatoryPlays();

        if(availablePlays == nullptr){
            availablePlays = getNonObligatoryPlays();
            if(availablePlays == nullptr){
                QString player(otherPlayer(m_player) == Alquerque::RedPlayer ? "Vermelho" : "Azul");
                QMessageBox::information(this, tr("Fim de jogo"), tr("Jogador %2 venceu.").arg(player));
                reset();
            }
            else{
                for(auto p : *availablePlays){
                    delete p;
                }
                delete availablePlays;
            }
        }
        else{
            for(auto p : *availablePlays){
                delete p;
            }
            delete availablePlays;
        }
    }

    removeMarkings();
    markSelectableHoles();
    // Finally, update the status bar.
    this->updateStatusBar();
}

/**
 * @brief Alquerque::holeInAvailableStartSelection Verifica se o hole informado esta na lista de jogadas disponiveis
 * Uma jogada disponivel eh uma jogada obrigatoria, se uma peca puder tomar uma peca inimiga, ou qualquer outra jogada valida
 * @param id
 * @return true, se o Hole informado pelo id esta na lista de jogadas disponiveis, false, caso contrario.
 */
bool Alquerque::holeInAvailableStartSelection(int id){
    bool retorno = false;
    auto availablePlays = getObligatoryPlays();

    if(availablePlays == nullptr){
        availablePlays = getNonObligatoryPlays();
    }

    if(availablePlays != nullptr){
        for(auto p : *availablePlays){
            if(p->initial == id){
                retorno = true;
            }
            delete p;
        }
    }

    delete availablePlays;
    return retorno;
}

/**
 * @brief Alquerque::holeInAvailableEndSelection Verifica se o par {start, end} correspondem a uma jogada disponivel
 * @param start
 * @param end
 * @return true, se end corresponde a um fim valido para uma jogada que comeca na posicao start, false, caso contrario
 */
bool Alquerque::holeInAvailableEndSelection(int start, int end){
    bool retorno = false;
    auto availablePlays = getObligatoryPlays();

    if(availablePlays == nullptr){
        availablePlays = getNonObligatoryPlays();
    }

    if(availablePlays != nullptr){
        for(auto p : *availablePlays){
            if(p->initial == start && p->final == end){
                retorno = true;
            }
            delete p;
        }
    }

    delete availablePlays;
    return retorno;
}

/**
 * @brief Alquerque::getObligatoryPlays Retorna uma lista das jogadas obrigatorias
 * @return um std::vector contendo todos os pares {inicio, fim} das jogadas obrigatorias, caso existam. nullptr caso nao existam jogadas obrigatorias
 */
std::vector<Alquerque::Plays*>* Alquerque::getObligatoryPlays(){
    //"tuplas" de jogadas obrigatorias: posicao inicial e posicao final
    std::vector<Alquerque::Plays*>* obligatoryPlays = new std::vector<Alquerque::Plays*>();

    const Hole::State otherPlayerState = player2state(otherPlayer(m_player));

    for(int id=0.; id<25; id++){
        const int row = id/5;
        const int col = id%5;
        if((m_player == Alquerque::RedPlayer && m_board[id/5][id%5]->state() == Hole::RedState) ||
                (m_player == Alquerque::BluePlayer && m_board[id/5][id%5]->state() == Hole::BlueState)){
            if(row-2 >=0 && m_board[row-2][col]->state() == Hole::EmptyState && m_board[row-1][col]->state() == otherPlayerState){
                obligatoryPlays->push_back(new Alquerque::Plays {id, id-10});
            }
            if(row+2 <= 4 && m_board[row+2][col]->state() == Hole::EmptyState && m_board[row+1][col]->state() == otherPlayerState){
                obligatoryPlays->push_back(new Alquerque::Plays {id, id+10});
            }
            if(col-2 >= 0 && m_board[row][col-2]->state() == Hole::EmptyState && m_board[row][col-1]->state() == otherPlayerState){
                obligatoryPlays->push_back(new Alquerque::Plays {id, id-2});
            }
            if(col+2 <= 4 && m_board[row][col+2]->state() == Hole::EmptyState && m_board[row][col+1]->state() == otherPlayerState){
                obligatoryPlays->push_back(new Alquerque::Plays {id, id+2});
            }

            if(id%2 == 0){ //buracos pares possuem diagonais
                if(row-2 >= 0 && col-2 >= 0 && m_board[row-2][col-2]->state() == Hole::EmptyState && m_board[row-1][col-1]->state() == otherPlayerState){
                    obligatoryPlays->push_back(new Alquerque::Plays {id, id-12});
                }
                if(row-2 >= 0 && col+2 <= 4 && m_board[row-2][col+2]->state() == Hole::EmptyState && m_board[row-1][col+1]->state() == otherPlayerState){
                    obligatoryPlays->push_back(new Alquerque::Plays {id, id-8});
                }
                if(row+2 <= 4 && col-2 >= 0 && m_board[row+2][col-2]->state() == Hole::EmptyState && m_board[row+1][col-1]->state() == otherPlayerState){
                    obligatoryPlays->push_back(new Alquerque::Plays {id, id+8});
                }
                if(row+2 <= 4 && col+2 <= 4 && m_board[row+2][col+2]->state() == Hole::EmptyState && m_board[row+1][col+1]->state() == otherPlayerState){
                    obligatoryPlays->push_back(new Alquerque::Plays {id, id+12});
                }
            }
        }
    }

    if(obligatoryPlays->size() == 0){
        delete obligatoryPlays;
        return nullptr;
    }
    return obligatoryPlays;
}

/**
 * @brief Alquerque::getNonObligatoryPlays etorna uma lista das jogadas nao obrigatorias
 * @return um std::vector contendo todos os pares {inicio, fim} das jogadas nao obrigatorias, caso existam. nullptr caso nao existam jogadas restantes
 * (fim de partida por deixar o outro jogador sem movimento)
 */
std::vector<Alquerque::Plays*>* Alquerque:: getNonObligatoryPlays(){
    //"tuplas" de jogadas obrigatorias: posicao inicial e posicao final (id)
    std::vector<Alquerque::Plays*>* nonObligatoryPlays = new std::vector<Alquerque::Plays*>();

    for(int id=0.; id<25; id++){
        const int row = id/5;
        const int col = id%5;
        if((m_player == Alquerque::RedPlayer && m_board[id/5][id%5]->state() == Hole::RedState) ||
        (m_player == Alquerque::BluePlayer && m_board[id/5][id%5]->state() == Hole::BlueState)){
            if(row-1 >= 0 && m_board[row-1][col]->state() == Hole::EmptyState){ //verificar hole acima
                nonObligatoryPlays->push_back(new Alquerque::Plays {id, id-5});
            }
            if(row+1 <= 4 && m_board[row+1][col]->state() == Hole::EmptyState){
                nonObligatoryPlays->push_back(new Alquerque::Plays {id, id+5});
            }
            if(col-1 >= 0 && m_board[row][col-1]->state() == Hole::EmptyState){
                nonObligatoryPlays->push_back(new Alquerque::Plays {id, id-1});
            }
            if(col+1 <= 4 && m_board[row][col+1]->state() == Hole::EmptyState){
                nonObligatoryPlays->push_back(new Alquerque::Plays {id, id+1});
            }

            if(id % 2 == 0){ //buracos pares possuem diagonais
                if(row-1 >= 0 && col-1 >= 0 && m_board[row-1][col-1]->state() == Hole::EmptyState){
                    nonObligatoryPlays->push_back(new Alquerque::Plays {id, id-6});
                }
                if(row-1 >= 0 && col+1 <= 4 && m_board[row-1][col+1]->state() == Hole::EmptyState){
                    nonObligatoryPlays->push_back(new Alquerque::Plays {id, id-4});
                }
                if(row+1 <= 4 && col-1 >= 0 && m_board[row+1][col-1]->state() == Hole::EmptyState){
                    nonObligatoryPlays->push_back(new Alquerque::Plays {id, id+4});
                }
                if(row+1 <= 4 && col+1 <= 4 && m_board[row+1][col+1]->state() == Hole::EmptyState){
                    nonObligatoryPlays->push_back(new Alquerque::Plays {id, id+6});
                }
            }
        }
    }

    if(nonObligatoryPlays->size() == 0){
        delete nonObligatoryPlays;
        return nullptr;
    }
    return nonObligatoryPlays;
}

/**
 * @brief Alquerque::markSelectableHoles Seta como "disabled" todos aqueles holes do jogador atual que nao sejam o inicio de uma jogada valida
 */
void Alquerque::markSelectableHoles(){

    Hole::State holeState = player2state(m_player);
    for(int i=0; i<5; i++){
        for(int j=0; j<5; j++){
            if(m_board[i][j]->state() == holeState){
                m_board[i][j]->setDisabled(true);
            }
            else{
                m_board[i][j]->setDisabled(false);
            }
        }
    }

        //verifica se o estado eh 3 para marcar a unica peca que pode ser movida, em um turno de 2 ou mais movimentos
        if(m_state != 3){
            auto availablePlays = getObligatoryPlays();

            if(availablePlays == nullptr){
                availablePlays = getNonObligatoryPlays();
            }
            for(auto p : *availablePlays){
                m_board[p->initial/5][p->initial%5]->setDisabled(false);
                delete p;
            }
            delete availablePlays;
        }
        else{
            m_board[m_last_played_hole_id/5][m_last_played_hole_id%5]->setDisabled(false);
        }
}

/**
 * @brief Alquerque::markAvailablePlays A partir de um id (hole) selecionado, marca todas as posicoes finais possiveis resultantes de uma jogada
 * @param id
 */
void Alquerque::markAvailablePlays(int id){
    auto availablePlays = getObligatoryPlays();

    if(availablePlays == nullptr){
        availablePlays = getNonObligatoryPlays();
    }

    if(availablePlays != nullptr){
        for(auto p : *availablePlays){
            if(p->initial == id){
                int row = p->final/5;
                int col = p->final %5;
                m_board[row][col]->setMarked(true);
            }
            delete p;
        }
    }

    delete availablePlays;
}

/**
 * @brief Alquerque::hasSubsequentPlay Verifica se para a nova posicao, existem mais jogadas obrigatorias possiveis
 * @param id
 * @return true, se existem mais jogadas disponiveis para aquela nova posicao, false, caso contrario
 */
bool Alquerque::hasSubsequentPlay(int id){
    bool retorno = false;
    auto obligatoryPlays = getObligatoryPlays();
    if(obligatoryPlays != nullptr){
        for(auto p : *obligatoryPlays){
            if(p->initial == id){
                retorno = true;
            }
            delete p;
        }
        delete obligatoryPlays;
    }
    return retorno;
}

/**
 * @brief Alquerque::removeMarkings Remove todas as marcacoes de pecas selecionadas e jogadas disponiveis
 */
void Alquerque::removeMarkings(){
    // Remove all markings.
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            Hole* hole = m_board[row][col];
            hole->setMarked(false);
        }
    }
}

/**
 * @brief Alquerque::reset Reseta o tabuleiro, as marcacoes e o estado do jogo
 */
void Alquerque::reset() {
    // Reset board.
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            Hole* hole = m_board[row][col];
            hole->reset();

            if (row < 2 || (row == 2 && col < 2))
                hole->setState(Hole::RedState);
            else if (row > 2 || (row == 2 && col > 2))
                hole->setState(Hole::BlueState);
        }
    }

    // Reset the player.
    m_player = Alquerque::RedPlayer;
    //Reset the play state
    m_state = 1;
    m_number_blue_pieces = 12;
    m_number_red_pieces = 12;

    markSelectableHoles();

    // Finally, update the status bar.
    this->updateStatusBar();
}

/**
 * @brief Alquerque::showAbout Cria uma QMessageBox contendo informacoes sobre a dupla que realizou o trabalho
 */
void Alquerque::showAbout() {
    QMessageBox::information(this, tr("About"), tr("Caio Vinícius Pereira Silveira - caiovpsilveira@gmail.com\nVinicius Hiago Golçalves - viniciushiago01@gmail.com"));
}

/**
 * @brief Alquerque::updateStatusBar Atualiza o status bar para informar que trocou o jogador
 */
void Alquerque::updateStatusBar() {
    QString player(m_player == Alquerque::RedPlayer ? "Vermelho" : "Azul");
    ui->statusbar->showMessage(tr("Vez do Jogador %2").arg(player));
}
