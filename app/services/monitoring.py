from typing import cast

from sqlalchemy import desc
from sqlalchemy.orm import Session

from app.models.incident import Incident
from app.models.reading import PowerReading
from app.schemas.reading import ReadingCreate

# constants for tuning
OVERLOAD_THRESHOLD = 500.0
LOW_VOLTAGE_THRESHOLD = 110.0
LOAD_AVG_WINDOW = 5
LOW_VOLTAGE_STREAK = 3


def create_incident(db: Session, node_id: int, itype: str, desc: str, severity: str = "high") -> None:
    incident = Incident(
        node_id=node_id,
        type=itype,
        severity=severity,
        description=desc,
    )
    db.add(incident)


def _recent_history(db: Session, node_id: int, limit: int) -> list[PowerReading]:
    return (
        db.query(PowerReading)
        .filter(PowerReading.node_id == node_id)
        .order_by(desc(PowerReading.timestamp))
        .limit(limit)
        .all()
    )


def _safe_average(values: list[float]) -> float:
    if not values:
        return 0.0
    return sum(values) / len(values)


def _has_recent_low_voltage_streak(voltages: list[float], streak_size: int, threshold: float) -> bool:
    # ordering from the newest
    if len(voltages) < streak_size:
        return False
    return all(v < threshold for v in voltages[:streak_size])


def process_reading(db: Session, reading_in: ReadingCreate) -> PowerReading:
    new_reading = PowerReading(**reading_in.model_dump())
    db.add(new_reading)

    # INSERT to the DB immediately!
    db.flush()

    history_window = max(LOAD_AVG_WINDOW, LOW_VOLTAGE_STREAK)
    history = _recent_history(db, reading_in.node_id, history_window)

    loads = [cast(float, r.load) for r in history]
    avg_load = _safe_average(loads)
    if avg_load > OVERLOAD_THRESHOLD:
        create_incident(
            db,
            reading_in.node_id,
            "overload",
            (
                f"average load over last {len(loads)} readings is "
                f"{avg_load:.2f}, above threshold {OVERLOAD_THRESHOLD:.2f}"
            ),
        )

    voltages = [cast(float, r.voltage) for r in history]
    if _has_recent_low_voltage_streak(voltages, LOW_VOLTAGE_STREAK, LOW_VOLTAGE_THRESHOLD):
        create_incident(
            db,
            reading_in.node_id,
            "voltage_drop",
            (
                f"last {LOW_VOLTAGE_STREAK} voltage readings are below "
                f"{LOW_VOLTAGE_THRESHOLD:.2f}V"
            ),
            severity="medium",
        )

    db.commit()
    db.refresh(new_reading)
    return new_reading
