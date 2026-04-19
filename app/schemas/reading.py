from pydantic import BaseModel, ConfigDict
from datetime import datetime

class ReadingCreate(BaseModel):
    node_id: int
    voltage: float
    load: float

class Reading(ReadingCreate):
    id: int
    timestamp: datetime
    model_config = ConfigDict(from_attributes=True)
