from sqlalchemy import Column, Integer, String, DateTime, ForeignKey
from sqlalchemy.sql import func
from app.db.session import Base

class Incident(Base):
    __tablename__ = "incidents"

    id = Column(Integer, primary_key=True, index=True)
    node_id = Column(Integer, ForeignKey("nodes.id"))
    timestamp = Column(DateTime(timezone=True), server_default=func.now())
    type = Column(String) # e.g., "overload", "voltage_drop"
    severity = Column(String) # e.g., "high", "medium"
    description = Column(String)
