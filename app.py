import os
import random
import time
from flask import Flask
from flask_sqlalchemy import SQLAlchemy
import hashlib
from sqlalchemy.sql import func
import uuid

CANDIDATE_SET_SIZE = 3200
MIN_GAMES_FOR_LEADERBOARD = 1
LATEST_GAMES_EVALUATED = 2

SALT = '3o0dkxm23ikd20dk203kdk02,xqmxqkoxWM'
basedir = os.path.abspath(os.path.dirname(__file__))
app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = \
    'sqlite:///' + os.path.join(basedir, 'database.db')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

def generate_uuid():
    return str(uuid.uuid4())

class User(db.Model):
    id = db.Column(db.String(64), primary_key=True, default=generate_uuid)
    username = db.Column(db.String(64), nullable=False, unique=True)
    games_played = db.Column(db.Integer, nullable=False)

class Game(db.Model):
    id = db.Column(db.String(64), primary_key=True)
    user = db.Column(db.String(32), nullable=False)
    wordlen = db.Column(db.Integer, default=0)
    seed = db.Column(db.Integer, nullable=False)
    solution = db.Column(db.String(8))
    guesses = db.Column(db.Integer, default=10)
    solved = db.Column(db.Boolean, default=False)
    created_at = db.Column(db.DateTime(timezone=True),
                           server_default=func.now())


def load_words(len=4):
    fname = "static/words" + str(len) + ".txt"
    with open(fname) as file:
        lines = file.read().splitlines()
    return lines


def choose_random_subset(list, size, seed):
    random.seed(seed)
    return random.sample(list, size)


def prepare_task(wordlen, solution_count, seed):
    return choose_random_subset(load_words(wordlen), CANDIDATE_SET_SIZE, seed)


@app.route('/')
def results():
    out = "<h1> Rules </h1>"
    out += "<p><b>Objective:</b> Create a bot to solve wordle puzzles as effectively as possible.</p>"
    out += "<p><b>Full problem statement and API description: </b><a href='/rules'>here</a></p><hr/>"
    out += "<h1>Ranking</h1>"
    for wlen in ('4', '5', '6'):
        was_empty = True
        out += "<h2>" + wlen + " letter words</h2>"
        with db.engine.connect() as con:
            rs = con.execute("SELECT U1.username as user, AVG(IIF(G1.solved,G1.guesses,10)) AS avg_score FROM Game G1 join User U1 on U1.id = G1.user WHERE G1.id in (SELECT G.id FROM Game G WHERE G.wordlen = "+str(wlen)+"  and G.user = G1.user ORDER BY G.created_at DESC LIMIT "+ str(LATEST_GAMES_EVALUATED) + ")  GROUP BY G1.user HAVING U1.games_played >= " + str(MIN_GAMES_FOR_LEADERBOARD) + " ORDER BY avg_score ASC")
            for row in rs:
                was_empty = False
                out += str(round(row.avg_score,3)) + " | " + row.user + "<br />"
        if was_empty:
            out += "No results yet."
    out += "<p><em>(Play at least " + str(
        MIN_GAMES_FOR_LEADERBOARD) + " games under a username to enter the leaderboards)</em></p>"
    out += "<hr/> <em> (c) Panaxeo 2022</em>"
    return out

@app.route('/register/<string:uname>')
def register(uname):
    exists = db.session.query(User.username).filter_by(username=uname).first() is not None
    if exists:
        return {'error':'User already registered!'}, 400

    user_obj = User(username=uname)
    db.session.add(user_obj)
    db.session.commit()
    return {'token': user_obj.id}, 200


@app.route('/start/<string:token>/<int:wordsize>')
def start_game(token, wordsize):
    user = token
    size = wordsize

    # validate input
    if size < 4 or size > 6:
        return {"error":"Incorrect word size."}, 400
    # todo return proper HTTP response

    user_obj = db.session.query(User).filter_by(id=token).first()
    if user_obj is None:
        return {'error': 'Unknown user token!'}, 400
    print(user_obj)
    user_obj.games_played = user_obj.games_played + 1
    db.session.add(user_obj)

    # generate game
    game_id = hashlib.md5((str(time.time()) + token + SALT).encode('utf-8')).hexdigest()
    seed = random.randint(0, 1000 * 1000 * 1000)
    candidate_solutions = prepare_task(size, CANDIDATE_SET_SIZE, seed)
    random.seed()
    solution = random.choice(candidate_solutions)

    new_game = Game(id=game_id, user=user, wordlen=size, seed=seed, solution=solution, guesses=0, solved=False)
    db.session.add(new_game)
    db.session.commit()

    assert game_id is not None
    response = {'gameid': game_id,
                'candidate_solutions': candidate_solutions}
    return response, 200


def evaluate_guess(solution, guess):
    assert len(solution) == len(guess)
    out_str = list(guess)
    for i in range(0, len(guess)):
        if solution[i] == guess[i]:
            out_str[i] = "Y"
        elif guess[i] in solution:
            out_str[i] = "_"
        else:
            out_str[i] = "N"
    return "".join(out_str)


def is_solved(solution_string):
    return "Y" * len(solution_string) == solution_string


@app.route('/guess/<string:game_id>/<string:guess>/')
def guess(game_id, guess):
    game_obj = Game.query.get(game_id)
    if game_obj is None:
        return {'error:':'Incorrect game id.'}, 400
    if game_obj.solved:
        return {'error':'Game already solved.'}, 410
    if game_obj.wordlen != len(guess):
        return {'error':'Wrong guess length.'}, 400

    game_obj.guesses = game_obj.guesses + 1
    solution = game_obj.solution
    guess_eval = evaluate_guess(solution, guess)

    if is_solved(guess_eval):
        game_obj.solved = True

    db.session.commit()
    return {'result':guess_eval}, 200


if __name__ == '__main__':
    app.run()
