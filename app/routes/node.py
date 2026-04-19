from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from app.db import deps
from app.models.node import GridNode
from app.schemas.node import NodeCreate, Node

router = APIRouter()

@router.post("/", response_model=Node)
def create_node(node_in: NodeCreate, db: Session = Depends(deps.get_db)):
    new_node = GridNode(name=node_in.name, location=node_in.location)
    
    db.add(new_node)
    db.commit()
    db.refresh(new_node)
    
    return new_node
