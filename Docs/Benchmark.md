# GTopia Benchmark (*October 20, 2026.*)

**500 simulated players on a single game server instance**

All numbers below were measured by the custom benchmark tooling, with the server and the bots running on the same machine.

> **About these results:** These numbers come from a synthetic load test, not from real players.
> The server and the bots ran on the same machine over loopback, so absolute figures will differ
> on other hardware and on a real network. The 100-bot run is the most reliable one; at 300 and
> 500 bots the benchmark client itself becomes the bottleneck, so those results show what the
> server handles *at least*, not its limit.

## At a glance

| | 100 players | 300 players | 500 players |
|---|---:|---:|---:|
| Server CPU | **0.21 cores** | 0.70 cores | 0.82 cores |
| Share of the whole machine | 1.3% | 4.4% | 5.2% |
| Memory (working set) | **46 MB** | 59 MB | 69 MB |
| Game thread busy | **0.23%** | 0.48% | 0.59% |
| Average game tick (20 ms budget) | **44 µs** | 92 µs | 117 µs |
| Ticks over budget | 0 | 0 | 0 |
| Failed logins | 0 | 0 | 0 |
| Unexpected disconnects | 0 | 0 | 0 |

> The 100-player run is the clean, fully reliable result. At 300 and 500 the benchmark client itself runs out of CPU first (see [Limits of running bots and server on one machine](#limits-of-running-bots-and-server-on-one-machine)), so those columns show the server holding up under at least that much load, not its limit.

## Test machine

| | |
|---|---|
| CPU | Intel Core i7 12th-gen H-series, 16 logical cores |
| OS | Windows |
| Layout | Server and all bots on the same machine, over loopback |

## What the bots do

Every bot behaves like a player and keeps going for the whole run:

| Behavior | Who | Details |
|---|---|---|
| **Login and join** | all | Logs in through the Master server (one new bot every 0.25 s), moves to the game server, and joins a world. |
| **Walking** | all | Walks left and right nonstop at about 200 px/s, turning around at random (about every 2 s) and at the world edges. Spawns at one of three heights. |
| **Position updates** | all | Sends a state packet every 200 ms while moving (5 per second). |
| **Farming** | 50% (builders) | One tile change every 200 ms (5 per second), alternating between placing dirt and breaking with a fist. |
| **Chatting** | all | Occasionally sends `hello world` (roughly one message per bot every couple of minutes). |
| **World hopping** | all | Leaves and joins a different world every ~3 minutes on average (60 s minimum plus a random delay). |

Worlds are assigned with a Zipf distribution: a few crowded worlds and many small ones.

| Run | Worlds | Bots in the busiest world at start |
|---|---:|---:|
| 100 bots | 7 | 37 |
| 300 bots | 20 | 75 |
| 500 bots | 34 | 128 |

## Packets and bandwidth

### Sent by the bots (to the server)

| | 100 players | 300 players | 500 players |
|---|---:|---:|---:|
| Per bot, at full speed | 5 packets/s (walkers), 10 packets/s (builders), 7.5 average | same | same |
| Expected total at full speed | 750 packets/s | 2,250 packets/s | 3,750 packets/s |
| **Measured total** | **742 packets/s** | 1,357 packets/s | 1,197 packets/s |

### Sent by the server (to the bots)

| | 100 players | 300 players | 500 players |
|---|---:|---:|---:|
| **Total** | **29,400 packets/s** | 162,800 packets/s | 157,100 packets/s |
| Per bot | ~294 packets/s | ~543 packets/s | ~314 packets/s |
| Bandwidth | 1.8 MB/s | 9.9 MB/s | 9.7 MB/s |
| Network thread cost | ~7 µs per packet | ~4 µs per packet | ~5 µs per packet |
| Network thread load | 20% of 1 core | 70% of 1 core | 82% of 1 core |
| Dropped packets (oversize / pool full) | 0 / 0 | 0 / 0 | 0 / 0 |

The 100-player row matches the expected rate exactly. At 300 and 500 the measured upload falls short of the expected total because the bot process is overloaded (see below).

### Why far more packets go out than come in

When a player moves or builds, the server sends that one packet to everyone else in the same world. At 100 bots, 742 packets/s in becomes about 29,400 out, roughly 40 times as many. The number of packets grows with the square of a world's population, so one crowded world dominates the load.

## Architecture notes

- **Separate threads** for game logic (fixed 20 ms ticks), networking (ENet), and database access, so a slow query or a busy socket never stalls the simulation.
- **One packet, many recipients.** Broadcasts create a single ENet packet and share it across all recipients instead of copying it per player.
- **Packet pool** with preallocated buffers, so no allocation on the hot path.
- **Per-connection rate limit** (200 packets/s, kick on abuse) and size limits on incoming packets.

## Limits of running bots and server on one machine

- **Shared CPU.** The bots and the server compete for the same cores. The bot process used 0.25 cores at 100 bots and about 1.0 core at 300 and 500, while the server used 0.2-0.8 cores. The whole machine stayed at 4-12% busy, so there was plenty of spare CPU; the limit is that the bot is one thread.
- **The bot is the first thing to saturate.** It is single-threaded. At 300 and 500 bots it sits at 100% of one core and falls behind, which is why the measured upload is only about 1.2-1.4k packets/s against the 2.3-3.8k the bots should send. Outgoing server traffic levels off near 160k packets/s for the same reason.
- **Loopback is not a real network.** There is no NIC, driver, or real-world network cost per packet, so a deployed server will spend more CPU per packet than the 4-7 µs measured here.
- **No distance, no real clients.** Bots are simple scripts. Real players are less constantly active (heavier in some ways, lighter in others), so treat the load as a stress test, not a prediction.