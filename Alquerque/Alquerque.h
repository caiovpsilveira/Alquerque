#ifndef ALQUERQUE_H
#define ALQUERQUE_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class Alquerque;
}
QT_END_NAMESPACE

class Hole;

class Alquerque : public QMainWindow {
    Q_OBJECT

public:
    enum Player {
        RedPlayer,
        BluePlayer
    };
    Q_ENUM(Player)

    Alquerque(QWidget *parent = nullptr);
    virtual ~Alquerque();

signals:
    void turnEnded();

private:
    Ui::Alquerque *ui;
    Player m_player;
    Hole* m_board[5][5];
    int m_state;
    int m_last_played_hole_id;
    int m_number_blue_pieces;
    int m_number_red_pieces;
    void removeMarkings();
    bool holeInAvailableStartSelection(int id);
    bool holeInAvailableEndSelection(int start, int end);
    void markAvailablePlays(int id);
    struct Plays{
        int initial;
        int final;
    };
    void markSelectableHoles();
    std::vector<Alquerque::Plays*>* getObligatoryPlays();
    std::vector<Alquerque::Plays*>* getNonObligatoryPlays();
    bool hasSubsequentPlay(int id);

private slots:
    void play(int id);
    void endTurn();
    void reset();

    void showAbout();
    void updateStatusBar();

};

#endif // ALQUERQUE_H
