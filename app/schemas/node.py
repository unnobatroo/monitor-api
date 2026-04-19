from pydantic import BaseModel, ConfigDict
from typing import Optional

class NodeCreate(BaseModel):
    name: str
    location: Optional[str] = None

class Node(NodeCreate):
    id: int
    model_config = ConfigDict(from_attributes=True)