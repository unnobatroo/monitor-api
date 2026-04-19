from sqlalchemy import Column, Integer, Float, DateTime, ForeignKey
from sqlalchemy.sql import func
from app.db.session import Base

class PowerReading(Base):
    __tablename__ = "readings"

    id = Column(Integer, primary_key=True, index=True)
    node_id = Column(Integer, ForeignKey("nodes.id"), nullable=False)
    
    voltage = Column(Float, nullable=False)
    load = Column(Float, nullable=False)
    timestamp = Column(DateTime(timezone=True), server_default=func.now())
