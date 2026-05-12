import chess
import chess.pgn
import sys
import io

if len(sys.argv) > 1:
    with open(sys.argv[1], "r") as f:
        game = chess.pgn.read_game(f)
else:
    game = chess.pgn.read_game(io.StringIO(sys.stdin.read()))

print(f"position fen {game.board().fen()} moves", end=" ")

for move in game.mainline_moves():
    print(move.uci(), end=" ")

print()