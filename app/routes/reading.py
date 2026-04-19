from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from app.db import deps
from app.models.node import GridNode
from app.schemas.reading import ReadingCreate, Reading
from app.services import monitoring

router = APIRouter()

@router.post("/", response_model=Reading)
def create_reading(reading_in: ReadingCreate, db: Session = Depends(deps.get_db)):
    node = db.query(GridNode).filter(GridNode.id == reading_in.node_id).first()
    if node is None:
        raise HTTPException(status_code=404, detail="Node not found")

    return monitoring.process_reading(db, reading_in)
