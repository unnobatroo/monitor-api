from sqlalchemy import Column, Integer, String
from app.db.session import Base

class GridNode(Base):
    __tablename__ = "nodes"
    id = Column(Integer, primary_key=True, index=True)
    name = Column(String, nullable=False)
    location = Column(String)